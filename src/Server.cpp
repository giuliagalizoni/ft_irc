#include "../includes/Server.hpp"
#include "../includes/SetupException.hpp"
#include "../includes/signals.hpp"

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <sstream>


static const int BUFFER_SIZE = 512;

// Creates the server socket, sets socket options, binds to the port, and starts listening.
Server::Server(int port, const std::string& password) : _port(port), _password(password)
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
		throw SetupException("Socket failed; couldn't create Server FD");
	fcntl(_serverFd, F_SETFL, O_NONBLOCK);

	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		close(_serverFd);
		throw SetupException("setsockopt failed");
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		close(_serverFd);
		throw SetupException("bind failed");
	}

	if (listen(_serverFd, SOMAXCONN) == -1)
	{
		close(_serverFd);
		throw SetupException("listen failed");
	}

	pollfd server_fd;
	server_fd.fd = _serverFd;
	server_fd.events = POLLIN;

	_fds.push_back(server_fd);

	std::cout << "Server listening in port " << _port << std::endl;
}

// Notifies all clients of shutdown, closes all file descriptors, and frees all User objects.
Server::~Server()
{
	for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		it->second->sendMessage("ERROR :Server is shutting down\r\n");
		it->second->flushOutput();
	}

	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd != -1)
			close(it->fd);
	}

	for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
		delete it->second;

	std::cout << "Server is closed, all Users are deleted. Bye 👋" << std::endl;
}

// Accepts a pending connection, sets it non-blocking, and registers the new User.
void Server::_acceptClient()
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	socklen_t addrlen = sizeof(addr);

	pollfd client;
	client.fd = accept(_serverFd, (struct sockaddr*)&addr, &addrlen);
	client.events = POLLIN;
	client.revents = 0;
	if (client.fd == -1)
	{
		std::cerr << "accept failed" << std::endl;
		return;
	}
	fcntl(client.fd, F_SETFL, O_NONBLOCK);
	_fds.push_back(client);
	_users[client.fd] = new User(client.fd, inet_ntoa(addr.sin_addr));

	std::cout << "client connected" << std::endl;
}

// Main poll loop: updates events, dispatches read/write, and processes deferred disconnects.
void Server::run()
{
	while (g_running)
	{
		for (size_t i = 0; i < _fds.size(); ++i)
		{
			if (_fds[i].fd == _serverFd)
			{
				_fds[i].events = POLLIN;
				continue;
			}

			_fds[i].events = POLLIN;

			std::map<int, User*>::iterator it = _users.find(_fds[i].fd);
			if (it != _users.end() && it->second->hasPendingOutput())
				_fds[i].events |= POLLOUT;
		}
		if (poll(&_fds[0], _fds.size(), -1) == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				std::cerr << "poll error" << std::endl;
				break;
			}
		}
		for (size_t i = 0; i < _fds.size(); i++)
		{
			short revents = _fds[i].revents;
			if (!revents)
				continue;
			if (_fds[i].fd == _serverFd)
			{
				if (revents & POLLIN)
					_acceptClient();
				continue;
			}
			if (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))
				_handleClient(_fds[i].fd);
			if (revents & POLLOUT)
			{
				std::map<int, User*>::iterator it = _users.find(_fds[i].fd);
				if (it != _users.end())
					it->second->flushOutput();
			}
		}
		std::vector<int> toRemove;
		for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
		{
			if (it->second->getDisconnecting() == true)
			toRemove.push_back(it->second->getFd());
		}

		for (size_t i = 0; i < toRemove.size(); ++i)
			_disconnectClient(toRemove[i]);

	}
}

// Reads data from a client fd, feeds it to the input buffer, and dispatches complete lines.
void Server::_handleClient(int fd)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	std::map<int, User*>::iterator it = _users.find(fd);
	if (it == _users.end())
		return ;
	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
	if (bytes <= 0)
	{
		it->second->setDisconnecting();
		std::cout << "client disconnected" << std::endl;
		return;
	}
	else
	{
		std::string line;
		User* user = it->second;

		user->appendToBuffer(std::string(buffer, bytes));
		while (user->getNextLine(line))
		{
			if (_processCommand(fd, line))
				return ;
		}
	}
	return;
}

// Parses one IRC line and dispatches it to the right handler.
// Unregistered users are blocked except for PASS, NICK, USER, CAP, and QUIT.
bool Server::_processCommand(int fd, std::string& line)
{
	Command cmd = _parseCommand(line);
	if (cmd.command.empty())
		return (false);

	User* user = _users[fd];
	if (!user->isRegistered()
		&& cmd.command != "PASS"
		&& cmd.command != "NICK"
		&& cmd.command != "USER"
		&& cmd.command != "CAP"
		&& cmd.command != "QUIT")
	{
		_sendNumeric(fd, "451", ":You have not registered");
		return (false);
	}

	if (cmd.command == "NICK")
		_handleNick(fd, cmd);
	else if (cmd.command == "JOIN")
		_handleJoin(fd, cmd);
	else if (cmd.command == "PASS")
		_handlePass(fd, cmd);
	else if (cmd.command == "USER")
		_handleUser(fd, cmd);
	else if (cmd.command == "PRIVMSG")
		_handlePrivmsg(fd, cmd);
	else if (cmd.command == "PART")
		_handlePart(fd, cmd);
	else if (cmd.command == "QUIT")
		_handleQuit(fd, cmd);
	else if (cmd.command == "TOPIC")
		_handleTopic(fd, cmd);
	else if (cmd.command == "INVITE")
		_handleInvite(fd, cmd);
	else if (cmd.command == "KICK")
		_handleKick(fd, cmd);
	else if (cmd.command == "MODE")
		_handleMode(fd, cmd);
	else if (cmd.command == "CAP")
	{
		if (!cmd.params.empty() && cmd.params[0] == "LS")
			_users[fd]->sendMessage(":ircserv CAP * LS :\r\n");
	}
	else if (cmd.command == "PING")
		_handlePing(fd, cmd);
	else if (cmd.command == "WHOIS")
		_handleWhois(fd, cmd);
	else
	{
		_sendNumeric(fd, "421", cmd.command + " :Unknown command");
	}
	return (false);
}

// Builds the nick!user@host prefix string for outgoing IRC messages.
std::string Server::_userPrefix(User* user) const
{
	return ":" + user->getNickname() + "!" + user->getUsername() + "@" + user->getHostname();
}

// Formats and sends an IRC numeric reply (e.g. 001, 433) to the given client.
void Server::_sendNumeric(int fd, const std::string& code, const std::string& text)
{
	std::string nick = _users[fd]->getNickname();

	if (nick.empty())
		nick = "*";

	std::string msg = ":ircserv " + code + " " + nick + " " + text + "\r\n";

	_users[fd]->sendMessage(msg);
}

// Handles JOIN: creates the channel if needed, enforces modes, and sends 353/366 name list.
void Server::_handleJoin(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "JOIN :Not enough parameters");
		return ;
	}

	User* user = _users[fd];

	std::string channelName = cmd.params[0];

	if (channelName == "#" || channelName.empty())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		return;
	}

	std::string key = "";

	if (cmd.params.size() >= 2)
		key = cmd.params[1];

	if (channelName[0] != '#')
		channelName = "#" + channelName;

	if (_channels.find(channelName) == _channels.end())
		_channels[channelName] = Channel(channelName);

	Channel& channel = _channels[channelName];

	if (channel.addUser(user, key))
	{
		std::cout << _users[fd]->getNickname() << " joined " << channelName << std::endl;
		std::string msg = _userPrefix(user) + " JOIN " + channelName + "\r\n";

		_broadcastToChannel(channel, msg, fd);
		_users[fd]->sendMessage(msg);

		if (channel.getTopic().empty())
			_sendNumeric(fd, "331", channelName + " :No topic is set");
		else
			_sendNumeric(fd, "332", channelName + " :" + channel.getTopic());

		std::string namesList;

		const std::set<User*>& users = channel.getUsers();
		for (std::set<User*>::const_iterator it = users.begin(); it != users.end(); ++it)
		{
			if (channel.isOperator(*it))
				namesList += "@";
			namesList += (*it)->getNickname();
			namesList += " ";
		}
		_sendNumeric(fd, "353", "= " + channelName + " :" + namesList);
		_sendNumeric(fd, "366", channelName + " :End of /NAMES list");
	}
	else
	{
		Channel::JoinResult reason = channel.canJoin(user, key);
		if (reason == Channel::JOIN_INVITEONLY)
			_sendNumeric(fd, "473", channelName + " :Cannot join channel (+i)");
		else if (reason == Channel::JOIN_BADKEY)
			_sendNumeric(fd, "475", channelName + " :Cannot join channel (+k)");
		else if (reason == Channel::JOIN_FULL)
			_sendNumeric(fd, "471", channelName + " :Cannot join channel (+l)");
		std::cout << _users[fd]->getNickname() << " could not join " << channelName << std::endl;
	}

	std::cout << "Channel users: " << channel.getUsers().size() << std::endl;
}

// Returns true if the nickname is valid per IRC rules (length, allowed characters, no leading digit).
static bool isValidNickname(const std::string& nick)
{
	if (nick.empty() || nick.size() > 30)
		return false;
	if (std::isdigit(static_cast<unsigned char>(nick[0])))
		return false;
	for (size_t i = 0; i < nick.size(); ++i)
	{
		char c = nick[i];
		if (!std::isalnum(static_cast<unsigned char>(c)) &&
			c != '-' && c != '_' && c != '[' && c != ']' &&
			c != '{' && c != '}' && c != '\\' && c != '|' &&
			c != '^' && c != '`')
			return false;
	}
	return true;
}


// Handles NICK: validates the new nickname, checks uniqueness, and notifies channel members on change.
void Server::_handleNick(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "431", ":No nickname given");
		return ;
	}

	std::string newNick = cmd.params[0];

	if (!isValidNickname(newNick))
	{
		_sendNumeric(fd, "432", newNick + " :Erroneous nickname");
		return ;
	}

	if (_nicknameExists(newNick, fd))
	{
		_sendNumeric(fd, "433", newNick + " :Nickname is already in use");
		return ;
	}

	bool registered = _users[fd]->isRegistered();
	std::string oldPrefix = _userPrefix(_users[fd]);
	_users[fd]->setNickname(newNick);

	if (registered)
	{
		std::string msg = oldPrefix + " NICK :" + newNick + "\r\n";
		_users[fd]->sendMessage(msg);
		for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
			if (it->second.hasUser(_users[fd]))
				_broadcastToChannel(it->second, msg, fd);
	}

	std::cout << "fd " << fd << " is now nicknamed " << _users[fd]->getNickname() << std::endl;

	_checkRegistration(fd);
}

// Sends a message to every member of the channel except the one with exceptFd.
void Server::_broadcastToChannel(Channel& channel, const std::string& msg, int exceptFd)
{
	const std::set<User*>& users = channel.getUsers();

	for (std::set<User*>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		if ((*it)->getFd() != exceptFd)
			(*it)->sendMessage(msg);
	}
}

// Splits a raw IRC line into a Command struct (command word + parameter list).
Command Server::_parseCommand(std::string& line)
{
	Command cmd;

	getCommand(cmd, line);
	getParams(cmd,line);

	return (cmd);
}

// Extracts and uppercases the command word from the front of line.
void Server::getCommand(Command& cmd, std::string& line)
{
	std::size_t found = line.find(" ");
	if (found != std::string::npos)
	{
		cmd.command = line.substr(0, found);
		line.erase(0, found + 1);
	}
	else
	{
		cmd.command = line;
		line.clear();
	}

	for(size_t i = 0; i < cmd.command.size(); i++)
		 cmd.command[i] = toupper(cmd.command[i]);
}

// Extracts all parameters from line into cmd.params, respecting the trailing ':' convention.
void Server::getParams(Command& cmd, std::string& line)
{
	while(!line.empty())
	{
		if (line[0] == ':')
		{
			cmd.params.push_back(line.substr(1));
			break;
		}
		std::size_t found = line.find(" ");
		if (found != std::string::npos)
		{
			if (found != 0)
				cmd.params.push_back(line.substr(0, found));
			line.erase(0, found + 1);
		}
		else
		{
			cmd.params.push_back(line);
			break;
		}
	}
}

// Handles PASS: checks the password and marks it received.
void Server::_handlePass(int fd, const Command& cmd)
{
	if (cmd.params.empty())
		return ;

	if (cmd.params[0] == _password)
	{
		_users[fd]->setPassReceived();
		std::cout << "Password accepted" << std::endl;
		_checkRegistration(fd);
	}
	else
	{
		_sendNumeric(fd, "464", ":Password incorrect");
	}
}

// Handles USER: sets the username and real name from the command parameters.
void Server::_handleUser(int fd, const Command& cmd)
{
	if (cmd.params.size() < 4)
	{
		_sendNumeric(fd, "461", "USER :Not enough parameters");
		return ;
	}

	User* user = _users[fd];

	user->setUsername(cmd.params[0]);

	std::string realname = cmd.params[3];
	user->setRealname(realname);
	user->setUserReceived();

	std::cout << "USER set for fd " << fd
				<< ": username=" << user->getUsername()
				<< ", realname=" << user->getRealname()
				<< std::endl;

	_checkRegistration(fd);
}

// Sends the 001 welcome message once PASS, NICK, and USER have all been received.
void Server::_checkRegistration(int fd)
{
	User* user = _users[fd];

	if (user->isRegistered() && !user->isRegistrationAnnounced())
	{
		user->setRegistrationAnnounced();

		std::string nick = user->getNickname();

		user->sendMessage(":ircserv 001 " + nick + " :Welcome to ft_irc\r\n");

		std::cout << nick << " registered" << std::endl;
	}
}

// Returns true if any connected user (other than exceptFd) holds the given nickname.
bool Server::_nicknameExists(const std::string& nickname, int exceptFd) const
{
	for (std::map<int, User*>::const_iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if (it->first != exceptFd && it->second->getNickname() == nickname)
			return (true);
	}
	return (false);
}

// Handles PRIVMSG: routes a message to a channel or directly to a user.
void Server::_handlePrivmsg(int fd, const Command& cmd)
{
	if (cmd.params.size() < 2)
	{
		_sendNumeric(fd, "461", "PRIVMSG :not enough parameters");
		return ;
	}

	User* user = _users[fd];

	std::string target = cmd.params[0];
	std::string msg = cmd.params[1];

	if (target.empty())
	{
		_sendNumeric(fd, "411", ":Empty recipient");
		return;
	}

	if (msg.empty())
	{
		_sendNumeric(fd, "412", ":No text to send");
		return;
	}

	if (target[0] == '#')
	{
		std::map<std::string, Channel>::iterator it = _channels.find(target);
		if (it == _channels.end())
		{
			_sendNumeric(fd, "403", target + " :No such channel");
			std::cout << "No such channel: " << target << std::endl;
			return ;
		}

		Channel& channel = it->second;

		if (!channel.hasUser(user))
		{
			_sendNumeric(fd, "404", target + " :Cannot send to channel");
			std::cout << "User is not in the channel: " << target << std::endl;
			return ;
		}

		std::string fullMsg = _userPrefix(user) + " PRIVMSG " + target + " :" + msg + "\r\n";
		_broadcastToChannel(channel, fullMsg, fd);
	}
	else
	{
		User* targetUser = _findUserByNickname(target);
		if (!targetUser)
		{
			_sendNumeric(fd, "401", target + " :No such nick");
			std::cout << "No such nick: " << target << std::endl;
			return ;
		}
		std::string fullMsg = _userPrefix(user) + " PRIVMSG " + target + " :" + msg + "\r\n";
		targetUser->sendMessage(fullMsg);
	}
}

// Returns a pointer to the connected User with the given nickname, or NULL if not found.
User* Server::_findUserByNickname(const std::string& nickname) const
{
	for (std::map<int, User*>::const_iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if (it->second->getNickname() == nickname)
			return (it->second);
	}
	return (NULL);
}

// Handles PART: removes the user from the channel and destroys the channel if it becomes empty.
void Server::_handlePart(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "PART :Not enough parameters");
		return ;
	}

	User* user = _users[fd];
	std::string channelName = cmd.params[0];

	std::map<std::string, Channel>::iterator it = _channels.find(channelName);
	if (it == _channels.end())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		std::cout << "PART: no such channel " << channelName << std::endl;
		return ;
	}

	Channel& channel = it->second;

	if (!channel.hasUser(user))
	{
		_sendNumeric(fd, "442", channelName + " :You're not on that channel");
		std::cout << "PART: user is not in channel " << channelName << std::endl;
		return ;
	}

	std::string msg = _userPrefix(user)
					+ " PART "
					+ channelName
					+ "\r\n";

	_broadcastToChannel(channel, msg, -1);

	channel.removeUser(user);

	std::cout << user->getNickname()
				<< " left "
				<< channelName
				<< std::endl;

	if (channel.getUsers().empty())
		_channels.erase(channelName);
}

// Handles QUIT: notifies all channels the user is in, then flags the client for disconnect.
bool Server::_handleQuit(int fd, const Command& cmd)
{
	User* user = _users[fd];

	std::string reason = "Client quit";
	if (!cmd.params.empty())
		reason = cmd.params[0];

	std::string msg = _userPrefix(user)
					+ " QUIT :"
					+ reason
					+ "\r\n";

	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		Channel& channel = it->second;

		if (channel.hasUser(user))
			_broadcastToChannel(channel, msg, fd);
	}

	_users[fd]->setDisconnecting();
	return (true);
}

// Handles TOPIC: sends the current topic (no argument) or sets a new one, respecting mode +t.
void Server::_handleTopic(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "TOPIC :Not enough parameters");
		return ;
	}

	User* user = _users[fd];
	std::string channelName = cmd.params[0];

	std::map<std::string, Channel>::iterator it = _channels.find(channelName);
	if (it == _channels.end())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		std::cout << "TOPIC: no such channel " << channelName << std::endl;
		return ;
	}

	Channel& channel = it->second;

	if (cmd.params.size() == 1)
	{
		if (channel.getTopic().empty())
			_sendNumeric(fd, "331", channelName + " :No topic is set");
		else
			_sendNumeric(fd, "332", channelName + " :" + channel.getTopic());
		std::cout << "Current topic for " << channelName << ": " << channel.getTopic() << std::endl;
		return ;
	}

	if (!channel.canChangeTopic(user))
	{
		_sendNumeric(fd, "482", channelName + " :You're not channel operator");
		std::cout << "TOPIC: user cannot change topic" << std::endl;
		return ;
	}

	std::string topic = cmd.params[1];
	channel.setTopic(topic);

	std::string msg = _userPrefix(user) + " TOPIC " + channelName + " :" + topic + "\r\n";

	_broadcastToChannel(channel, msg, -1);
}

// Handles INVITE: checks operator status and adds the target to the channel invite list.
void Server::_handleInvite(int fd, const Command& cmd)
{
	if (cmd.params.size() < 2)
	{
		_sendNumeric(fd, "461", "INVITE :Not enough parameters");
		return ;
	}

	User* inviter = _users[fd];

	std::string targetNick = cmd.params[0];
	std::string channelName = cmd.params[1];

	User* invitedUser = _findUserByNickname(targetNick);
	if (!invitedUser)
	{
		_sendNumeric(fd, "401", targetNick + " :No such nick");
		std::cout << "INVITE: no such nick " << targetNick << std::endl;
		return ;
	}

	std::map<std::string, Channel>::iterator it = _channels.find(channelName);
	if (it == _channels.end())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		std::cout << "INVITE: no such channel " << channelName << std::endl;
		return ;
	}

	Channel& channel = it->second;

	if (!channel.hasUser(inviter))
	{
		_sendNumeric(fd, "442", channelName + " :You're not on that channel");
		std::cout << "INVITE: inviter is not in the channel " << channelName << std::endl;
		return ;
	}
	if (!channel.isOperator(inviter))
	{
		_sendNumeric(fd, "482", channelName + " :You're not channel operator");
		std::cout << "INVITE: inviter is not operator" << std::endl;
		return ;
	}
	if (channel.hasUser(invitedUser))
	{
		_sendNumeric(fd, "443", targetNick + " " + channelName + " :is already on channel");
		std::cout << "INVITE: user already in channel " << channelName << std::endl;
		return ;
	}

	channel.inviteUser(invitedUser);

	std::string msg = ":" + inviter->getNickname() + " INVITE " + invitedUser->getNickname() + " " + channelName + "\r\n";

	invitedUser->sendMessage(msg);

	_sendNumeric(fd, "341", invitedUser->getNickname() + " " + channelName);

	std::cout << inviter->getNickname() << " invited " << invitedUser->getNickname() << " to " << channelName << std::endl;
}

// Handles KICK: checks operator status and removes the target from the channel.
void Server::_handleKick(int fd, const Command& cmd)
{
	if (cmd.params.size() < 2)
	{
		_sendNumeric(fd, "461", "KICK :Not enough parameters");
		return ;
	}

	User* kicker = _users[fd];

	std::string channelName = cmd.params[0];
	std::string targetNick = cmd.params[1];

	std::map<std::string, Channel>::iterator it = _channels.find(channelName);
	if (it == _channels.end())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		std::cout << "KICK: no such channel" << std::endl;
		return;
	}

	Channel& channel = it->second;

	if (!channel.hasUser(kicker))
	{
		_sendNumeric(fd, "442", channelName + " :You're not on that channel");
		std::cout << "KICK: kicker is not in the channel " << channelName << std::endl;
		return ;
	}
	if (!channel.isOperator(kicker))
	{
		_sendNumeric(fd, "482", channelName + " :You're not channel operator");
		std::cout << "KICK: kicker is not operator" << std::endl;
		return ;
	}

	User *target = _findUserByNickname(targetNick);
	if (!target)
	{
		_sendNumeric(fd, "401", targetNick + " :No such nick");
		std::cout << "KICK: no such nick" << std::endl;
		return ;
	}

	if (!channel.hasUser(target))
	{
		_sendNumeric(fd, "441", targetNick + " " + channelName + " :They aren't on that channel");
		std::cout << "KICK: user not in the channel " << channelName << std::endl;
		return ;
	}

	std::string msg = ":" + kicker->getNickname() + " KICK " + channelName + " " + targetNick + "\r\n";

	_broadcastToChannel(channel, msg, -1);

	channel.removeUser(target);

	std::cout << kicker->getNickname() << " kicked " << targetNick << " from " << channelName << std::endl;
}

// Handles MODE: applies or queries channel modes i, t, k, l, o; ignores user-mode queries.
void Server::_handleMode(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "MODE :Not enough parameters");
		return ;
	}

	User* user = _users[fd];

	std::string channelName = cmd.params[0];
	if (channelName.empty() || channelName[0] != '#')
	{
		_sendNumeric(fd, "221", "+");
		return;
	}

	std::map<std::string, Channel>::iterator it = _channels.find(channelName);
	if (it == _channels.end())
	{
		_sendNumeric(fd, "403", channelName + " :No such channel");
		std::cout << "MODE: no such channel" << channelName << std::endl;
		return ;
	}

	Channel& channel = it->second;

	if (cmd.params.size() == 1)
	{
		_sendNumeric(fd, "324", channelName + " " + channel.getModeString());
		std::cout << "Current modes: " << channel.getModeString() << std::endl;
		return ;
	}

	std::string mode = cmd.params[1];

	if (mode == "b" || mode == "+b" || mode == "-b")
	{
		_sendNumeric(fd, "368", channelName + " :End of channel ban list");
		return ;
	}

	if (!channel.hasUser(user))
	{
		_sendNumeric(fd, "442", channelName + " :You're not on that channel");
		std::cout << "MODE: user is not in the channel" << std::endl;
		return ;
	}

	if (!channel.isOperator(user))
	{
		_sendNumeric(fd, "482", channelName + " :You're not channel operator");
		std::cout << "MODE: user is not operator" << std::endl;
		return ;
	}

	if (mode.size() != 2 || (mode[0] != '+' && mode[0] != '-'))
	{
		std::cout << "MODE: invalid mode format" << std::endl;
		return ;
	}

	bool adding = (mode[0] == '+');
	char flag = mode[1];

	if (flag == 'i')
		channel.setInviteOnly(adding);
	else if (flag == 't')
		channel.setTopicRestricted(adding);
	else if (flag == 'k')
	{
		if (adding)
		{
			if (cmd.params.size() < 3)
			{
				_sendNumeric(fd, "461", "MODE :Not enough parameters");
				std::cout << "MODE +k: missing key" << std::endl;
				return ;
			}
			channel.setKey(cmd.params[2]);
		}
		else
			channel.removeKey();
	}
	else if (flag == 'l')
	{
		if (adding)
		{
			if (cmd.params.size() < 3)
			{
				_sendNumeric(fd, "461", "MODE :Not enough parameters");
				std::cout << "MODE +l: missing limit" << std::endl;
				return ;
			}
			long limit;
			std::istringstream iss(cmd.params[2]);
			iss >> limit;
			if  (iss.fail() || !iss.eof() || limit <= 0)
			{
				std::cout << "MODE +l: invalid limit" << std::endl;
				return ;
			}
			channel.setUserLimit(limit);
		}
		else
			channel.removeUserLimit();
	}
	else if (flag == 'o')
	{
		if (cmd.params.size() < 3)
		{
			_sendNumeric(fd, "461", "MODE :Not enough parameters");
			std::cout << "MODE +/-o: missing nick" << std::endl;
			return ;
		}

		User* target = _findUserByNickname(cmd.params[2]);
		if (!target || !channel.hasUser(target))
		{
			_sendNumeric(fd, "441", cmd.params[2] + " " + channelName + " :They aren't on that channel");
			std::cout << "MODE +/- o: target not in channel" << std::endl;
			return ;
		}

		if (adding)
			channel.addOperator(target);
		else
			channel.removeOperator(target);
	}
	else
	{
		_sendNumeric(fd, "472", std::string(1, flag) + " :is unknown mode char to me");
		std::cout << "MODE: unknown mode" << flag << std::endl;
		return ;
	}

	std::string msg = _userPrefix(user) + " MODE " + channelName + " " + mode;
	if ((flag == 'k' || flag == 'l' || flag == 'o') && cmd.params.size() >= 3)
		msg = msg + " " + cmd.params[2];

	msg = msg + "\r\n";

	_broadcastToChannel(channel, msg, -1);
}

// Removes a client from all channels, the poll list, and the user map; closes the fd and frees memory.
void Server::_disconnectClient(int fd)
{
	User* user = _users[fd];
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); )
	{
		Channel& channel = it->second;
		channel.removeUser(user);

		if (channel.getUsers().empty())
			_channels.erase(it++);
		else
			++it;
	}

	for (size_t i = 0; i < _fds.size(); ++i)
	{
		if (_fds[i].fd == fd)
		{
			_fds.erase(_fds.begin() + i);
			break;
		}
	}
	close(fd);
	delete user;
	_users.erase(fd);
}

// Handles PING: replies with PONG to keep the connection alive.
void Server::_handlePing(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "PING :Not enough parameters");
		return;
	}
	_users[fd]->sendMessage(":ircserv PONG ircserv :" + cmd.params[0] + "\r\n");
}

// Handles WHOIS: sends 311 with user info and 318 to mark the end of the reply.
void Server::_handleWhois(int fd, const Command& cmd)
{
	if (cmd.params.empty())
	{
		_sendNumeric(fd, "461", "WHOIS :Not enough parameters");
		return;
	}
	User* target = _findUserByNickname(cmd.params[0]);
	if (!target)
	{
		_sendNumeric(fd, "401", cmd.params[0] + " :No such nick");
		return;
	}
	std::string nick = target->getNickname();
	std::string user = target->getUsername();
	std::string host = target->getHostname();
	std::string real = target->getRealname();
	_users[fd]->sendMessage(":ircserv 311 " + _users[fd]->getNickname()
		+ " " + nick + " " + user + " " + host + " * :" + real + "\r\n");
	_users[fd]->sendMessage(":ircserv 318 " + _users[fd]->getNickname()
		+ " " + nick + " :End of WHOIS list\r\n");
}
