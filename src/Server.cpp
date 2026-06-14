#include "../includes/Server.hpp"
#include "../includes/SetupException.hpp"

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cctype>

static const int BUFFER_SIZE = 512;


// TODO: break this logic into helpers
Server::Server(int port, const std::string& password) : _port(port), _password(password)
{
	// TODO: port should be already validated
	// create listening IPv4 TCP socket with non-blocking mode
	_serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_serverFd == -1)
		throw SetupException("Socket failed; couldn't create Server FD");

	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)  // allow the address/port to be reused sooner
	{
		close(_serverFd);
		throw SetupException("setsockopt failed");
	}
	// binding socket
	struct sockaddr_in addr; // IPv4 socket address structure
	memset(&addr, 0, sizeof(addr)); // initialize struct with zeros
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(_port); // convert the port to network byte order.
	addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any interface

	if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) // bind socket to address
	{
		close(_serverFd);
		throw SetupException("bind failed");
	}

	if (listen(_serverFd, SOMAXCONN) == -1)// mark the socket as passive (waiting for connections)
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

Server::~Server()
{
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		// iterator behaves like a pointer
		if (it->fd != -1)
			close(it->fd);
	}

	// delete all users
	for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
		delete it->second; // iterators for maps have first and second

	std::cout << "Server is closed, all Users are deleted. Bye 👋" << std::endl;
}

void Server::_acceptClient()
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	socklen_t addrlen = sizeof(addr);

	pollfd client;
	client.fd = accept4(_serverFd, (struct sockaddr*)&addr, &addrlen, SOCK_NONBLOCK);
	client.events = POLLIN; // tell poll to watch this fd for incoming data
	client.revents = 0; // initialize the returned events field to zero, so it won't be mistakenly processed in the current poll iteration before poll has checked it
	// TODO: implement exception
	if (client.fd == -1)
	{
		std::cerr << "accept4 failed" << std::endl;
		return;
	}
	_fds.push_back(client);
	_users[client.fd] = new User(client.fd, inet_ntoa(addr.sin_addr));

	std::cout << "client connected" << std::endl;
}

void Server::run()
{
	// poll
	while (1)
	{
		if (poll(&_fds[0], _fds.size(), -1) == -1)
		{
			std::cerr << "poll error" << std::endl;
			continue; // TODO: implement exception
		}
		for (size_t i = 0; i < _fds.size(); i++)
		{
			if (!(_fds[i].revents & POLLIN))
				continue;
			if (_fds[i].fd == _serverFd)
				_acceptClient();
			else
			{
				if (_handleClient(_fds[i].fd))
				{
					_fds.erase(_fds.begin() + i);
					i--;
				}
			}
		}
	}
}

bool Server::_handleClient(int fd)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0); // read data from socket into buffer
	// TODO: implement exceptions
	if (bytes <= 0) // in case recv fails . recv() can return -1 too, so bytes <= 0
	{
		close(fd);
		delete _users[fd];
		_users.erase(fd);
		if (bytes == 0)
			std::cout << "client disconnected" << std::endl;
		else if (bytes == -1)
			std::cerr << "recv failed" << std::endl; // TODO exception
		return true;
	}
	else // in case it works
	{
		std::string line;

		_users[fd]->appendToBuffer(std::string(buffer, bytes));
		while (_users[fd]->getNextLine(line))
		{
			//std::cout << line << std::endl;.... in order to generalise it:
			_processCommand(fd, line);
		}

		return false;

	}
}

void Server::_processCommand(int fd, std::string& line)
{
	Command cmd = _parseCommand(line);
	if (cmd.command.empty())
		return ;

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

	// still more commands to come

	else
	{
		std::cout << "Unknown command: " << cmd.command << std::endl;
	}
}

void Server::_handleJoin(int fd, const Command& cmd)		// IRC command = text ending with \r\n
{
	if (cmd.params.empty())
		return ;

	User* user = _users[fd];		//just a pointer to that particular client

	std::string channelName = cmd.params[0];

	if (channelName[0] != '#')
		channelName = "#" + channelName;

	if (_channels.find(channelName) == _channels.end())		//_channels.end() means it didnt find any in the iteration
		_channels[channelName] = Channel(channelName);		// creates the Channel object and store it under channelName

	Channel& channel = _channels[channelName];				// just a reference to the channelName object

	if (channel.addUser(user, ""))// "" key as placeholder
	{
		std::cout << _users[fd]->getNickname() << " joined " << channelName << std::endl;

		std::string nick = _users[fd]->getNickname();

		if (nick.empty())		//defensive, because already handled in handleNick
			nick = "*";

		std::string msg = ":" + nick + " JOIN " + channelName + "\r\n";

		_broadcastToChannel(channel, msg, fd);
		_users[fd]->sendMessage(msg);
	}
	else
	{
		std::cout << _users[fd]->getNickname() << " could not join " << channelName << std::endl;
	}

	//some debug
	std::cout << "Channel users: " << channel.getUsers().size() << std::endl;
}

void Server::_handleNick(int fd, const Command& cmd)
{
	if (cmd.params.empty())
		return ;

	std::string newNick = cmd.params[0];
	if (_nicknameExists(newNick, fd))
	{
		std::cout << "Nickname already in use: " << newNick << std::endl;
		_users[fd]->sendMessage(":ircserv 433 * " + newNick + " :Nickname is already in use \r\n");	// error 433 is historic convention for nickname in use
		return ;
	}
	_users[fd]->setNickname(newNick);

	std::cout << "fd " << fd << " is now nicknamed " << _users[fd]->getNickname() << std::endl;

	_checkRegistration(fd);
}

void Server::_broadcastToChannel(Channel& channel, const std::string& msg, int exceptFd)
{
	const std::set<User*>& users = channel.getUsers();

	for (std::set<User*>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		if ((*it)->getFd() != exceptFd)			// the exceptFd is the one of the just joined user, to avoid sending the message to him, which would send it duplicated.
			(*it)->sendMessage(msg);
	}
}

Command Server::_parseCommand(std::string& line)
{
	Command cmd;

	getCommand(cmd, line);
	getParams(cmd,line);

	return (cmd);
}

void Server::getCommand(Command& cmd, std::string& line)
{
	std::size_t found = line.find(" "); // find the first space
	if (found != std::string::npos) // if there's a space
	{
		cmd.command = line.substr(0, found); // extract the command up to the space
		line.erase(0, found + 1); // clean the extracted part from line
	}
	else // no space
		cmd.command = line; // take all the line

	for(size_t i = 0; i < cmd.command.size(); i++) // covert it to upper case to be sure
		 cmd.command[i] = toupper(cmd.command[i]);
}

void Server::getParams(Command& cmd, std::string& line)
{
	while(!line.empty()) // while there's still something remaining in line
	{
		if (line[0] == ':') // if it's starts with :
		{
			cmd.params.push_back(line.substr(1)); // add everything from after the :
			break; // and exit the loop
		}
		std::size_t found = line.find(" "); // find the next space
		if (found != std::string::npos) // it there's one
		{
			if (found != 0)
				cmd.params.push_back(line.substr(0, found)); // push everything up to the space to params vector
			line.erase(0, found + 1); // clean the extracted part
		}
		else // if there's no space and line is not empty, it's the last parameter
		{
			cmd.params.push_back(line); // push everything in line
			break; // exit the loop
		}
	}
}

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
		std::cout << "Wrong password" << std::endl;
	}
}

void Server::_handleUser(int fd, const Command& cmd)	// USER charlie 0 * :Charlie Chaplin
{
	if (cmd.params.size() < 4)
	{
		std::cout << "USER: not enough parameters" << std::endl;
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

void Server::_checkRegistration(int fd)
{
	User* user = _users[fd];

	if (user->isRegistered() && !user->isRegistrationAnnounced())
	{
		user->setRegistrationAnnounced();

		std::cout << user->getNickname() << " registered" << std::endl;
	}
}

bool Server::_nicknameExists(const std::string& nickname, int exceptFd) const
{
	for (std::map<int, User*>::const_iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if (it->first != exceptFd && it->second->getNickname() == nickname)	// we have to avoid the current user, so exceptFd in the first part of the pair. The second is the comparison with the nickname
			return (true);
	}
	return (false);
}

void Server::_handlePrivmsg(int fd, const Command& cmd)			//similar to handleUser. To parse something like: PRIVMSG #test :hello charlie
{
	if (cmd.params.size() < 2)
	{
		std::cout << "PRIVMSG: not enough parameters" << std::endl;
		return ;
	}

	User* user = _users[fd];

	std::string target = cmd.params[0];
	std::string msg = cmd.params[1];	//but actually is from 3 on...

	if (target[0] == '#')
	{
		// channel message. send message to everyboyd in the channel except the own user... :Groucho PRIVMSG #test :hello charlie
		// to broadcast we need the channel object, not just its name
		std::map<std::string, Channel>::iterator it = _channels.find(target);
		if (it == _channels.end())
		{
			std::cout << "No such channel: " << target << std::endl;
			return ;
		}

		Channel& channel = it->second;			// key: #test; value: Channel("#test")

		if (!channel.hasUser(user))
		{
			std::cout << "User is not in the channel: " << target << std::endl;
			return ;
		}

		std::string fullMsg = ":" + user->getNickname() + " PRIVMSG " + target + " :" + msg + "\r\n";
		_broadcastToChannel(channel, fullMsg, fd);
	}
	else		// direct message
	{
		User* targetUser = _findUserByNickname(target);
		if (!targetUser)
		{
			std::cout << "No such nick: " << target << std::endl;
			return ;
		}
		std::string fullMsg = ":" + user->getNickname() + " PRIVMSG " + target + " :" + msg + "\r\n";
		targetUser->sendMessage(fullMsg);
	}
}

User* Server::_findUserByNickname(const std::string& nickname) const
{
	for (std::map<int, User*>::const_iterator it = _users.begin(); it != _users.end(); ++it)
	{
		if (it->second->getNickname() == nickname)
			return (it->second);
	}
	return (NULL);
}
