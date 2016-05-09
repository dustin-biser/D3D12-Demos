#pragma once

#include <exception>

/// General purpose exception for use with D3D12-Demo code.
class DemoException : public std::exception {
public:

	DemoException()
		: errorMessage(nullptr)
	{}

	DemoException(const char * message)
		: errorMessage(message)
	{}

	virtual ~DemoException() {}

	const char * what() const noexcept
	{
		return errorMessage;
	}

private:
	const char * errorMessage;
};


/// Denotes a failed operation due to lack of available memory.
class InsufficientMemory : DemoException {
public:
	InsufficientMemory() {}

	InsufficientMemory(const char * message) 
		: DemoException(message) {}
};

