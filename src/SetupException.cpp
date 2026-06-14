#include "../includes/SetupException.hpp"

SetupException::SetupException(const std::string& msg) : _msg(msg) {}

SetupException::~SetupException() throw() {}

const char* SetupException::what() const throw()
{
	return _msg.c_str();
}
