#include "../includes/User.hpp"

#include <iostream>
#include <string>
#include <sys/socket.h>

User::User(int fd, const std::string& hostname)
	: 	_fd(fd),
		_buffer(""),
		_hostname(hostname),
		_nickname(""),
		_username(""),
		_realname(""),
		_passReceived(false),
		_nickReceived(false),
		_userReceived(false),
		_registrationAnnounced(false),
		_disconnecting(false)
{
	std::cout << "User " << _fd << " created" << std::endl;
}

User::~User()
{
	std::cout << "User " << _fd << " deleted" << std::endl;
}

// appends raw bytes to _buffer
void User::appendToBuffer(const std::string& data)
{
	_buffer.append(data);
}

// extracts one `\r\n`-terminated line from `_buffer`, returns false if none available
bool User::getNextLine(std::string& line)
{
	std::size_t found = _buffer.find("\n");

	if (found == std::string::npos)
		return false;
	else
	{
		line = _buffer.substr(0, found);
		_buffer.erase(0, found + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.end() - 1, line.end());
		return true;
	}
}

void User::sendMessage(const std::string& msg)
{
	_outBuffer += msg;
}

bool User::hasPendingOutput() const
{
	return !_outBuffer.empty();
}

void User::flushOutput()
{
	ssize_t bytesSent =  0;

	if (_outBuffer.empty())
		return;

	bytesSent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
	if (bytesSent > 0)
		_outBuffer.erase(0, bytesSent);
	else if (bytesSent == -1)
		_disconnecting = true;
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

void User::setNickname(const std::string& nickname)
{
	_nickname = nickname;
	setNickReceived();
}

void User::setUsername(const std::string& username)
{
	_username = username;
	setUserReceived();
}

void User::setRealname(const std::string& realname)
{
	_realname = realname;
}

bool User::isRegistrationAnnounced()
{
	return (_registrationAnnounced);
}

void User::setRegistrationAnnounced()
{
	_registrationAnnounced = true;
}

void User::setDisconnecting()
{
	_disconnecting = true;
}

bool User::getDisconnecting() const
{
	return _disconnecting;
}

