#include "communication.hpp"

namespace communication
{
	void dispatch_event(json event)
	{
		EVENT_CODE code = event["event"];

		switch (code)
		{
			case EVENT_CODE::ADD_BREAKPOINT:
			case EVENT_CODE::REMOVE_BREAKPOINT:
			{
				auto& data = event["data"];
				uintptr_t address = data["address"];

				if (code == EVENT_CODE::ADD_BREAKPOINT)
					debugger::add_breakpoint(address);
				else
					debugger::remove_breakpoint(address);

				break;
			}
			case EVENT_CODE::PAUSE:
				break;
			case EVENT_CODE::RESUME:

				debugger::continue_execution((debugger::DEBUG_ACTION)code);

				break;
			case EVENT_CODE::STEP_IN:
				break;
			case EVENT_CODE::STEP_OVER:
				debugger::continue_execution((debugger::DEBUG_ACTION)code);
				break;

			case EVENT_CODE::REGISTER_WRITE:
				
				auto data = event["data"];
				auto registerName = data["register"];
				uintptr_t value = data["value"];
				auto reg = util::parse_register(registerName);

				if (reg == nullptr) break;
				
				debugger::add_register_write(reg, value);
				break;
		}
	}

	void listen_thread()
	{
		while(true)
		{
			std::string message = pipe::receive();

			json event = json::parse(message);
			dispatch_event(event);
		}
	}

	void breakpoint_callback(EXCEPTION_POINTERS* ex)
	{
		CONTEXT* ctx = ex->ContextRecord;
		json event = {
			{ "event", (int)EVENT_CODE::BP_HIT },
			{ "data", {

					{ "Address", (uintptr_t)ex->ExceptionRecord->ExceptionAddress },
					{ "Rip", ctx->Rip },
					{ "Rsp", ctx->Rsp },
					{ "Rbp", ctx->Rbp },
					{ "Rdi", ctx->Rdi },
					{ "Rsi", ctx->Rsi },
					{ "Rbx", ctx->Rbx },
					{ "Rdx", ctx->Rdx },
					{ "Rcx", ctx->Rcx },
					{ "Rax", ctx->Rax },
					{ "R8", ctx->R8 },
					{ "R9", ctx->R9 },
					{ "R10", ctx->R10 },
					{ "R11", ctx->R11 },
					{ "R12", ctx->R12 },
					{ "R13", ctx->R13 },
					{ "R14", ctx->R14 },
					{ "R15", ctx->R15 },
					{ "EFlags", ctx->EFlags }
			} }
		};

		pipe::send(event.dump());
	}

	void start_listening()
	{
		// Create a thread that listens for messages
		std::thread(listen_thread).detach();
	}
}