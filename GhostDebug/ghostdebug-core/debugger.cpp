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

	std::vector <register_write_cmd> register_write_queue;
	
	void apply_register_writes(CONTEXT* ctx)
	{
		for (register_write_cmd& cmd : register_write_queue)
		{
			auto reg = cmd.reg;
			ctx->*reg = cmd.value;
		}
	}

	LONG single_step_handler(EXCEPTION_POINTERS* exception_info, DWORD thread_id)
	{
		CONTEXT* ctx = exception_info->ContextRecord;
		std::unique_lock<std::mutex> lock(single_step_mutex);
		if (thread_single_step.find(thread_id) != thread_single_step.end())
		{
			if (thread_single_step[thread_id])
			{
				// this thread is in single step mode
				communication::breakpoint_callback(exception_info);
				breakpoint_hit = true;

				resume_cv.wait(lock, [] { return user_action != DEBUG_ACTION::NONE; });

				auto action = user_action;
				user_action = DEBUG_ACTION::NONE;

				breakpoint_hit = false;

				apply_register_writes(ctx);

				// handle user action
				switch (action)
				{
				case DEBUG_ACTION::RESUME:
				{
					thread_single_step[thread_id] = false;
					CLEAR_STEP_FLAG(ctx); // Resume normal execution

					return EXCEPTION_CONTINUE_EXECUTION;
				}
				case DEBUG_ACTION::STEP_OVER:
				{
					SET_STEP_FLAG(ctx); // Set the single step flag to restore int3 next instruction
					return EXCEPTION_CONTINUE_EXECUTION;
				}
				case DEBUG_ACTION::STEP_IN:
					break;
				case DEBUG_ACTION::STEP_OUT:
					break;
				}

				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}

	// Central exception handler
	// anything that triggers an exception (MessageBox, OutputDebugString, etc) 
	// must be treated with caution here to avoid infinite loops
	LONG WINAPI exception_handler(EXCEPTION_POINTERS* exception_info)
	{
		DWORD thread_id = GetCurrentThreadId();
		uintptr_t exception_address = (uintptr_t)exception_info->ExceptionRecord->ExceptionAddress;
		CONTEXT* ctx = exception_info->ContextRecord;

		if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			// Check if the breakpoint is in our list
			if (breakpoints.find(exception_address) != breakpoints.end())
			{
				std::unique_lock<std::mutex> lock(breakpoint_mutex);
				auto bp = breakpoints[exception_address];

				// send context to client
				communication::breakpoint_callback(exception_info);
				breakpoint_hit = true;

				resume_cv.wait(lock, [] { return user_action != DEBUG_ACTION::NONE; });

				auto action = user_action;
				user_action = DEBUG_ACTION::NONE;

				breakpoint_hit = false;

				apply_register_writes(ctx);

				// handle user action
				switch (action)
				{
					case DEBUG_ACTION::RESUME:
					{
						bp->disable();

						// Relate the current breakpoint to this thread to restore the int3 in the single step
						std::lock_guard<std::mutex> step_over_lock(thread_restore_mutex);
						thread_restore_breakpoints[thread_id] = bp;
						
						bp->single_step = false;
						thread_single_step[thread_id] = false;
						
						ctx->Rip = exception_address; // Reset execution to original breakpoint address
						SET_STEP_FLAG(ctx); // Set the single step flag to restore int3 next instruction

						return EXCEPTION_CONTINUE_EXECUTION;
					}
					case DEBUG_ACTION::STEP_OVER:
					{
						bp->disable();
						bp->single_step = true;
						// Relate the current breakpoint to this thread to restore the int3 in the single step
						std::lock_guard<std::mutex> step_over_lock(thread_restore_mutex);
						
						thread_restore_breakpoints[thread_id] = bp;
						
						ctx->Rip = exception_address; // Reset execution to original breakpoint address
						SET_STEP_FLAG(ctx); // Set the single step flag to restore int3 next instruction

						return EXCEPTION_CONTINUE_EXECUTION;
					}
					case DEBUG_ACTION::STEP_IN:
						break;
					case DEBUG_ACTION::STEP_OUT:
						break;
				}

				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}

		if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
		{
			{
				std::lock_guard<std::mutex> step_lock(thread_restore_mutex);

				// check if this thread has a stored step-over breakpoint
				auto it = thread_restore_breakpoints.find(thread_id);
				if (it != thread_restore_breakpoints.end())
				{
					auto bp = thread_restore_breakpoints[thread_id];
					bp->enable();

					if (bp->single_step)
					{
						// breakpoint has been restored, so no need to single step anymore
						bp->single_step = false;
						SET_STEP_FLAG(ctx); // Set the single step flag to restore int3 next instruction
						thread_single_step[thread_id] = true;
						single_step_handler(exception_info, thread_id);

					}
					else
					{
						CLEAR_STEP_FLAG(ctx);
					}
					thread_restore_breakpoints.erase(it); // we can clear this now
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			single_step_handler(exception_info, thread_id);

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	bool init()
	{
		// Catch all exceptions
		return AddVectoredExceptionHandler(1, exception_handler) != NULL;
	}

	void add_breakpoint(uintptr_t address)
	{
		breakpoints[address] = std::make_shared<breakpoint>(address);
		breakpoints[address]->enable();
	}

	void remove_breakpoint(uintptr_t address)
	{
		// check if breakpoint exists
		if (breakpoints.find(address) == breakpoints.end())
			return;

		breakpoints[address]->disable();
		breakpoints.erase(address);
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