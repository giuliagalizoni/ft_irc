#include "../includes/User.hpp"

#include <iostream>
#include <string>
#include <sys/socket.h>

// Initialises a new user with their fd and hostname; all registration flags start as false.
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

// Destructor.
User::~User()
{
	std::cout << "User " << _fd << " deleted" << std::endl;
}

// Appends raw received bytes to the input buffer.
void User::appendToBuffer(const std::string& data)
{
	_buffer.append(data);
}

// Extracts one CRLF-terminated line from the buffer; returns false if none is available yet.
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

// Queues a message in the output buffer to be sent on the next POLLOUT event.
void User::sendMessage(const std::string& msg)
{
	_outBuffer += msg;
}

// Returns true if there is data in the output buffer waiting to be sent.
bool User::hasPendingOutput() const
{
	return !_outBuffer.empty();
}

// Sends as much of the output buffer as possible; marks the user for disconnect on send error.
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

// Returns the client socket file descriptor.
int User::getFd() const
{
	return _fd;
}

// Returns true when PASS, NICK, and USER have all been received.
bool User::isRegistered() const
{
	if (_passReceived && _nickReceived && _userReceived)
		return true;
	return false;
}

// Returns the current nickname.
std::string User::getNickname() const
{
	return _nickname;
}

// Returns the username.
std::string User::getUsername() const
{
	return _username;
}

// Returns the real name.
std::string User::getRealname() const
{
	return _realname;
}

// Returns the hostname (taken from accept).
std::string User::getHostname() const
{
	return _hostname;
}

// Marks the PASS step of registration as complete.
void User::setPassReceived()
{
	_passReceived = true;
}

// Marks the NICK step of registration as complete.
void User::setNickReceived()
{
	_nickReceived = true;
}

// Marks the USER step of registration as complete.
void User::setUserReceived()
{
	_userReceived = true;
}

// Sets the nickname and marks NICK as received.
void User::setNickname(const std::string& nickname)
{
	_nickname = nickname;
	setNickReceived();
}

// Sets the username and marks USER as received.
void User::setUsername(const std::string& username)
{
	_username = username;
	setUserReceived();
}

// Sets the real name.
void User::setRealname(const std::string& realname)
{
	_realname = realname;
}

// Returns true if the 001 welcome message has already been sent.
bool User::isRegistrationAnnounced()
{
	return (_registrationAnnounced);
}

// Marks that the 001 welcome message has been sent.
void User::setRegistrationAnnounced()
{
	_registrationAnnounced = true;
}

// Flags this user for deferred disconnection at the end of the current poll iteration.
void User::setDisconnecting()
{
	_disconnecting = true;
}

// Returns true if this user is pending disconnection.
bool User::getDisconnecting() const
{
	return _disconnecting;
}
