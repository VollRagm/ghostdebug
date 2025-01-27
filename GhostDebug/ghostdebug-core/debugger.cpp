#include "debugger.hpp"

class breakpoint
{
public:
	uintptr_t address = 0;
	std::vector<uint8_t> original_instruction = std::vector<uint8_t>();
	bool enabled = false;
	bool single_step = false;

	breakpoint(uintptr_t address)
	{
		this->address = address;
	}

	breakpoint() = default;

	void enable()
	{
		if (enabled)
			return;

		util::place_int3(address, original_instruction);
		enabled = true;
	}

	void disable()
	{
		if (!enabled)
			return;

		util::remove_int3(address, original_instruction);
		enabled = false;
	}
};

struct register_write_cmd
{
	DWORD64 CONTEXT::* reg;
	DWORD64 value;
};

namespace debugger
{
    std::unordered_map<uintptr_t, std::shared_ptr<breakpoint>> breakpoints;
    std::mutex breakpoint_mutex;
    std::condition_variable resume_cv;
    DEBUG_ACTION user_action = DEBUG_ACTION::NONE;
    bool breakpoint_hit = false;

    std::unordered_map<DWORD, std::shared_ptr<breakpoint>> thread_restore_breakpoints;
    std::mutex thread_restore_mutex;

    std::unordered_map<DWORD, bool> thread_single_step;
    std::mutex single_step_mutex;

    std::vector<register_write_cmd> register_write_queue;

    // apply_register_writes applies any register modifications
    // queued by add_register_write before continuing the thread.
    void apply_register_writes(CONTEXT* ctx)
    {
        for (auto& cmd : register_write_queue)
        {
            ctx->*(cmd.reg) = cmd.value;
        }
    }

    // The caller must hold a lock before calling this function.
    template <typename LockType>
    DEBUG_ACTION wait_for_debug_action(EXCEPTION_POINTERS* exception_info, CONTEXT* ctx, LockType& lock)
    {
        // send context to client
        communication::breakpoint_callback(exception_info);
        breakpoint_hit = true;

        // wait for user to pick an action
        resume_cv.wait(lock, [] { return user_action != DEBUG_ACTION::NONE; });

        // store then clear user_action
        auto action = user_action;
        user_action = DEBUG_ACTION::NONE;
        breakpoint_hit = false;

        // apply queued register writes
        apply_register_writes(ctx);

        return action;
    }

    LONG handle_single_step(EXCEPTION_POINTERS* exception_info, DWORD thread_id)
    {
        CONTEXT* ctx = exception_info->ContextRecord;

        {
            // check if this thread has a stored step-over breakpoint
            std::lock_guard<std::mutex> step_lock(thread_restore_mutex);

            auto it = thread_restore_breakpoints.find(thread_id);
            if (it != thread_restore_breakpoints.end())
            {
                // check if we have a breakpoint to restore
                auto bp = it->second;
                bp->enable();

                // if single_step is set, we continue single stepping;
                // otherwise, we clear it
                if (bp->single_step)
                {
                    // breakpoint has been restored, so no need to keep single_step flag on the bp
                    bp->single_step = false;
                    SET_STEP_FLAG(ctx);           // Set the single step flag to restore int3 next instruction
                    thread_single_step[thread_id] = true;
                }
                else
                {
                    CLEAR_STEP_FLAG(ctx);
                    thread_restore_breakpoints.erase(it);
                    return EXCEPTION_CONTINUE_EXECUTION;
                }

                thread_restore_breakpoints.erase(it);
            }
        }

        // if the thread is in single step mode, handle the callback/wait logic.
        std::unique_lock<std::mutex> lock(single_step_mutex);
        auto single_step_iter = thread_single_step.find(thread_id);

        // this thread is in single step mode
        if (single_step_iter != thread_single_step.end() && single_step_iter->second)
        {
            auto action = wait_for_debug_action(exception_info, ctx, lock);

            // handle user action
            switch (action)
            {
            case DEBUG_ACTION::RESUME:
            {
                thread_single_step[thread_id] = false;
                CLEAR_STEP_FLAG(ctx); // Resume normal execution
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            case DEBUG_ACTION::STEP_INTO:
            {
                SET_STEP_FLAG(ctx); // Set single step flag to restore int3 next instruction
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            case DEBUG_ACTION::STEP_OVER:
                break;
            case DEBUG_ACTION::STEP_OUT:
                break;
            }
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Central exception handler
    // anything that triggers an exception (MessageBox, OutputDebugString, etc)
    // must be treated with caution here to avoid infinite loops
    LONG WINAPI exception_handler(EXCEPTION_POINTERS* exception_info)
    {
        DWORD thread_id = GetCurrentThreadId();
        uintptr_t exception_address = (uintptr_t)exception_info->ExceptionRecord->ExceptionAddress;
        CONTEXT* ctx = exception_info->ContextRecord;

        // Check if it's an INT3
        if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
        {
            // Check if the breakpoint is in our list
            auto bp_it = breakpoints.find(exception_address);
            if (bp_it != breakpoints.end())
            {
                std::unique_lock<std::mutex> lock(breakpoint_mutex);
                auto bp = bp_it->second;

                // disable breakpoint to allow inspection without int3
                bp->disable();

                // call our unified wait function (callback->wait->apply_register_writes)
                auto action = wait_for_debug_action(exception_info, ctx, lock);

                // re-enable breakpoint when inspection is complete
                bp->enable();

                // handle user action
                switch (action)
                {
                case DEBUG_ACTION::RESUME:
                {
                    bp->disable();

                    // relate the current breakpoint to this thread to restore the int3 in the single step
                    {
                        std::lock_guard<std::mutex> step_over_lock(thread_restore_mutex);
                        thread_restore_breakpoints[thread_id] = bp;
                    }

                    bp->single_step = false;
                    thread_single_step[thread_id] = false;

                    // Reset execution to original breakpoint address
                    ctx->Rip = exception_address;
                    // Set the single step flag to restore int3 next instruction
                    SET_STEP_FLAG(ctx);

                    return EXCEPTION_CONTINUE_EXECUTION;
                }
                case DEBUG_ACTION::STEP_INTO:
                {
                    bp->disable();
                    bp->single_step = true;

                    // relate the current breakpoint to this thread to restore the int3 in the single step
                    {
                        std::lock_guard<std::mutex> step_over_lock(thread_restore_mutex);
                        thread_restore_breakpoints[thread_id] = bp;
                    }

                    ctx->Rip = exception_address; // Reset execution to original breakpoint address
                    SET_STEP_FLAG(ctx);           // Set the single step flag to restore int3 next instruction
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
                case DEBUG_ACTION::STEP_OVER:
                    break;
                case DEBUG_ACTION::STEP_OUT:
                    break;
                }
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }

        // Check if it's a single step
        if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
            return handle_single_step(exception_info, thread_id);

        // otherwise, not handled by us
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Catch all exceptions
    bool init()
    {
        return AddVectoredExceptionHandler(1, exception_handler) != NULL;
    }

    void add_breakpoint(uintptr_t address)
    {
        auto bp = std::make_shared<breakpoint>(address);
        breakpoints[address] = bp;
        bp->enable();
    }

    void remove_breakpoint(uintptr_t address)
    {
        // check if breakpoint exists
        auto it = breakpoints.find(address);
        if (it == breakpoints.end())
            return;

        it->second->disable();
        breakpoints.erase(it);
    }

    void add_register_write(DWORD64 CONTEXT::* reg, DWORD64 value)
    {
        register_write_cmd cmd;
        cmd.reg = reg;
        cmd.value = value;
        register_write_queue.push_back(cmd);
    }

    void continue_execution(DEBUG_ACTION action)
    {
        std::lock_guard<std::mutex> lock(breakpoint_mutex);
        if (breakpoint_hit)
        {
            user_action = action;
            resume_cv.notify_one();
        }
    }
}