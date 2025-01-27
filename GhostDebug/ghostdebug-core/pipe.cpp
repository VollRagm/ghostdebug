#include "pipe.hpp"

namespace pipe
{
	HANDLE hPipe;

    // Utility function to wait for an overlapped operation
    void wait_for_io(OVERLAPPED& overlapped)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            // Wait until the operation is complete
            WaitForSingleObject(overlapped.hEvent, INFINITE);
            ResetEvent(overlapped.hEvent);
        }
    }

    // Create a named pipe in asynchronous mode
    HANDLE create(std::string name)
    {
        hPipe = CreateNamedPipeA(
            name.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // Overlapped mode for async
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            1024 * 16,
            1024 * 16,
            0, // Default timeout
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            LOG("Failed to create named pipe: %d", GetLastError());
            return INVALID_HANDLE_VALUE;
        }

        // Set up the overlapped structure
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!overlapped.hEvent)
        {
            LOG("Failed to create event for overlapped operation: %d", GetLastError());
            CloseHandle(hPipe);
            return INVALID_HANDLE_VALUE;
        }

        // Attempt to connect to the named pipe
        BOOL result = ConnectNamedPipe(hPipe, &overlapped);
        if (!result)
        {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING)
            {
                DWORD waitResult = WaitForSingleObject(overlapped.hEvent, INFINITE);
                if (waitResult != WAIT_OBJECT_0)
                {
                    LOG("Failed to wait for pipe connection: %d", GetLastError());
                    CloseHandle(hPipe);
                    CloseHandle(overlapped.hEvent);
                    return INVALID_HANDLE_VALUE;
                }
            }
            else if (error != ERROR_PIPE_CONNECTED)
            {
                LOG("Failed to connect named pipe: %d", error);
                CloseHandle(hPipe);
                CloseHandle(overlapped.hEvent);
                return INVALID_HANDLE_VALUE;
            }
        }

        LOG("Named pipe created and connected successfully.");
        CloseHandle(overlapped.hEvent); // Clean up the event
        return hPipe;
    }

    // Send a message asynchronously
    void send(std::string message)
    {
        if (hPipe == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        if(!overlapped.hEvent)
			return;

        BOOL success = WriteFile(hPipe, message.c_str(), (DWORD)message.size(), &written, &overlapped);

        if (!success && GetLastError() != ERROR_IO_PENDING)
            return;
        else
            wait_for_io(overlapped);

        CloseHandle(overlapped.hEvent);
    }

    // Receive a full message asynchronously
    std::string receive()
    {
        if (hPipe == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Pipe not initialized!");

        char buffer[1024];
        ZeroMemory(buffer, sizeof(buffer));

        DWORD bytesRead = 0;
        std::string message;
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!overlapped.hEvent) return "";

        while (true)
        {
            BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, &overlapped);

            if (!success)
            {
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    // Wait for the operation to complete
                    wait_for_io(overlapped);
                    GetOverlappedResult(hPipe, &overlapped, &bytesRead, FALSE);
                    message.append(buffer, bytesRead);
                    break;
                }
                else if (GetLastError() == ERROR_MORE_DATA)
                {
                    // Append partial data and continue reading
                    message.append(buffer, bytesRead);
                }
                else
                {
                    CloseHandle(overlapped.hEvent);
                }
            }
            else
            {
                // Append the last chunk of the message and break
                if (bytesRead > 0)
                    message.append(buffer, bytesRead);
                break;
            }
        }

        CloseHandle(overlapped.hEvent);
        return message;
    }


}