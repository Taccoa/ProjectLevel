#include "CircBuffer.h"
#include <conio.h>
#include <tchar.h>
#include <stdio.h>

class Mutex {
private:
	HANDLE handle;
public:
	Mutex(LPCWSTR name)
	{
		handle = CreateMutex(nullptr, false, name);
	}
	~Mutex()
	{
		ReleaseMutex(handle);
	}
	void Lock(DWORD milliseconds = INFINITE)
	{
		WaitForSingleObject(handle, milliseconds);
	}
	void Unlock()
	{
		ReleaseMutex(handle);
	}
};

CircularBuffer::CircularBuffer() {}

CircularBuffer::CircularBuffer(LPCWSTR buffName, const size_t & buffSize, const bool & isProducer, const size_t & chunkSize)
{
	control = new size_t[4];

	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, buffSize, buffName);
	if (hMapFile == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), GetLastError());
	}

	controlFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(control), L"control");
	if (controlFileMap == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), GetLastError());
	}

	messageData = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, buffSize);
	if (messageData == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"), GetLastError());
		CloseHandle(hMapFile);
	}

	controlData = (size_t*)MapViewOfFile(controlFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(control));
	if (controlData == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"), GetLastError());
		CloseHandle(controlFileMap);
	}

	head = controlData;
	tail = head + 1;
	clients = tail + 1;
	freeMemory = clients + 1;

	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		*head = 0;
		*tail = 0;
		*clients = 0;
		*freeMemory = buffSize;
	}
	
	if (isProducer == false)
	{
		Mutex myMutex(L"myMutex");
		myMutex.Lock();
		*clients += 1;
		myMutex.Unlock();
		inTail = 0;
	}

	this->chunkSize = chunkSize;
	bufferSize = buffSize;
	msgID = 1;
}

CircularBuffer::~CircularBuffer()
{
	CloseHandle(hMapFile);
	CloseHandle(controlFileMap);
	UnmapViewOfFile(messageData);
	UnmapViewOfFile(controlData);
	delete[] control;
}

bool CircularBuffer::push(const void * msg, size_t length)
{
	size_t message_head = length + sizeof(Header);
	size_t remaining = message_head % chunkSize;
	size_t padding = chunkSize - remaining;
	size_t messageSize = sizeof(Header) + length + padding;
	size_t memoryLeft = bufferSize - *head;

	if (messageSize > memoryLeft)
	{
		if (*tail != 0)
		{
			size_t dummySize = memoryLeft - sizeof(Header);

			Header header{ 0, dummySize, 0, *clients };
			memcpy(messageData + *head, &header, sizeof(Header));
			Mutex myMutex(L"myMutex");
			myMutex.Lock();
			*freeMemory -= (dummySize + sizeof(Header));
			*head = 0;
			myMutex.Unlock();
			return false;
		}
		else
			return false;
	}
	else if (messageSize < (*freeMemory - 1))
	{
		Header header{ msgID++, length, padding, *clients };
		memcpy(messageData + *head, &header, sizeof(Header));
		memcpy(messageData + *head + sizeof(Header), msg, length);
		Mutex myMutex(L"myMutex");
		myMutex.Lock();
		*freeMemory -= messageSize;
		*head = (*head + messageSize) % bufferSize;
		myMutex.Unlock();
		return true;
	}
	else
	{
		return false;
	}
}

bool CircularBuffer::pop(char * msg, size_t & length)
{
	if (*freeMemory < bufferSize)
	{
		if (*head != inTail)
		{
			Header* h = (Header*)(&messageData[inTail]);
			length = h->length;
			size_t messageSize = sizeof(Header) + h->length + h->padding;
			if (h->id == 0)
			{
				inTail = 0;
				Mutex myMutex(L"myMutex");
				myMutex.Lock();
				h->consumersLeft -= 1;
				if (h->consumersLeft == 0)
				{
					*freeMemory += messageSize;
					*tail = inTail;
				}
				myMutex.Unlock();
				return false;
			}
			else
			{
				memcpy(msg, &messageData[inTail + sizeof(Header)], h->length);
				inTail = (inTail + messageSize) % bufferSize;
				Mutex myMutex(L"myMutex");
				myMutex.Lock();
				h->consumersLeft -= 1;
				if (h->consumersLeft == 0)
				{
					*freeMemory += messageSize;
					*tail = inTail;
				}
				myMutex.Unlock();
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}