#include "pipe.hpp"

namespace pipe
{
	HANDLE hPipe;

	HANDLE create(std::string name)
	{
		// Create a pipe to send and receive messages
		// Use TYPE_MESSAGE to send messages in a single block
		HANDLE hp = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			1, 1024 * 16, 1024, NMPWAIT_USE_DEFAULT_WAIT, nullptr);

		if (hp == INVALID_HANDLE_VALUE)
		{
			LOG("Failed to create pipe server: %d", GetLastError());
			return INVALID_HANDLE_VALUE;
		}
			

		if (!ConnectNamedPipe(hp, nullptr))
		{
			DWORD error = GetLastError();
			if (error != ERROR_PIPE_CONNECTED)
			{
				LOG("Failed to connect to pipe server: %d", error);
				CloseHandle(hp);
				return INVALID_HANDLE_VALUE;
			}
		}

		LOG("Pipe server created!");

		hPipe = hp;
		return hp;
	}

	void send(std::string message)
	{
		DWORD written;
		WriteFile(hPipe, message.c_str(), (DWORD)message.size(), &written, nullptr);
	}

	// Blocking call that receives a full message
	std::string receive()
	{
		char buffer[1024];
		DWORD bytesRead;
		std::string message;

		while (true)
		{
			bool success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr);

			if (!success)
			{
				// Partial message was read
				if(GetLastError() == ERROR_MORE_DATA)
					message.append(buffer, bytesRead);
				else
					throw std::runtime_error("Failed to read from pipe");
			}
			else
			{
				if(bytesRead > 0)
					message.append(buffer, bytesRead);
				break;
			}
		}

		return message;
	}


}