#include <Windows.h>

#ifndef CIRCBUFFER_H
#define CIRCBUFFER_H

class CircularBuffer
{
private:
	struct Header
	{
		size_t id;
		size_t length;
		size_t padding;
		int consumersLeft;
	};

	char* messageData;
	size_t* controlData;
	size_t* control;

	size_t* head;
	size_t* tail;
	size_t* clients;
	size_t* freeMemory;
	size_t inTail;

	size_t bufferSize;
	size_t chunkSize;
	
	size_t msgID;

	HANDLE hMapFile;
	HANDLE controlFileMap;

public:
	CircularBuffer();
	CircularBuffer(LPCWSTR buffName, const size_t& buffSize, const bool& isProducer, const size_t& chunkSize);
	~CircularBuffer();
					   
	bool push(const void* msg, size_t length);
	bool pop(char* msg, size_t& length);
};
#endif