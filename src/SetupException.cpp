#include "../includes/SetupException.hpp"

// Stores the error message.
SetupException::SetupException(const std::string& msg) : _msg(msg) {}

// Destructor.
SetupException::~SetupException() throw() {}

// Returns the stored error message as a C string.
const char* SetupException::what() const throw()
{
	return _msg.c_str();
}
