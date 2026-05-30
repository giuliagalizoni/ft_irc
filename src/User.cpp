#include "../includes/User.hpp"

#include <iostream>
#include <string>
#include <sys/socket.h>


User::User(int fd, const std::string& hostname): _fd(fd), _hostname(hostname)
{
	std::cout << "User " << _fd << "created" << std::endl;
}

User::~User()
{
	std::cout << "User " << _fd << "deleted" << std::endl;
}

// appends raw bytes to _buffer
void User::appendToBuffer(const std::string& data)
{
	_buffer.append(data);
}

// extracts one `\r\n`-terminated line from `_buffer`, returns false if none available
bool User::getNextLine(std::string& line)
{
	std::size_t found = _buffer.find("\r\n");

	if (found == std::string::npos)
		return false;
	else
	{
		line = _buffer.substr(0, found);
		_buffer.erase(0, found + 2);
		return true;
	}
}

void User::sendMessage(const std::string& msg)
{
	ssize_t bytesSent =  0;

	while (bytesSent < msg.size())
	{
		ssize_t result = send(_fd, msg.c_str() + bytesSent, msg.size() - bytesSent, 0);
		if (result == -1)
		{
			std::cerr << "send error" << std::endl;
			break;
		}
		bytesSent += result;
	}

	// TODO: implememnt exceptions

}


int User::getFd() const
{
	return _fd;
}

bool User::isRegistered() const
{
	if (_passReceived && _nickReceived && _userReceived)
		return true;
	return false;
}

std::string User::getNickname() const
{
	return _nickname;
}

std::string User::getUsername() const
{
	return _username;
}

std::string User::getRealname() const
{
	return _realname;
}

std::string User::getHostname() const
{
	return _hostname;
}

void User::setPassReceived()
{
	_passReceived = true;
}

void User::setNickReceived()
{
	_nickReceived = true;
}

void User::setUserReceived()
{
	_userReceived = true;
}
