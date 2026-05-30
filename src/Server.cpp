#include "../includes/Server.hpp"

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

static const int BUFFER_SIZE = 512;


// TODO: break this logic into helpers
Server::Server(int port, const std::string& password) : _port(port), _password(password)
{
	// TODO: port should be already validated
	// create listening IPv4 TCP socket with non-blocking mode
	_serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_serverFd == -1)
	{
		close(_serverFd);
		std::cerr << "Couldn't creat socket" << std::endl;
		//TODO:replace with exception
	}

	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)  // allow the address/port to be reused sooner
	{
		close(_serverFd);
		std::cerr << "setsockopt failed" << std::endl;
		// TODO: exception;
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
		std::cerr << "bind failed" << std::endl;
		// TODO: exception;
	}

	if (listen(_serverFd, SOMAXCONN) == -1)// mark the socket as passive (waiting for connections)
	{
		close(_serverFd);
		std::cerr << "listen failed" << std::endl;
		// TODO: exception;
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
	if (bytes == 0) // in case recv fails
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
			std::cout << line << std::endl;
		return false;

	}
}
