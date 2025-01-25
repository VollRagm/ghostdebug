#include "communication.hpp"

enum class EVENT_CODE
{
	ADD_BREAKPOINT,
	REMOVE_BREAKPOINT,
	PAUSE,
	RESUME,
	STEP_IN,
	STEP_OVER

};


namespace communication
{
	void dispatch_event(json event)
	{
		EVENT_CODE code = event["event"];

		switch (code)
		{
			case EVENT_CODE::ADD_BREAKPOINT:
			case EVENT_CODE::REMOVE_BREAKPOINT:

				auto& data = event["data"];
				uintptr_t address = data["address"];
				
				if(code == EVENT_CODE::ADD_BREAKPOINT)
					debugger::add_breakpoint(address);
				else
					debugger::remove_breakpoint(address);

				break;
			case EVENT_CODE::PAUSE:
				break;
			case EVENT_CODE::RESUME:
				break;
			case EVENT_CODE::STEP_IN:
				break;
			case EVENT_CODE::STEP_OVER:
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

	void start_listening()
	{
		// Create a thread that listens for messages
		std::thread listener(listen_thread);
	}
}