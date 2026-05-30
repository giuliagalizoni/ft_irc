#include "../includes/Server.hpp"

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>


// TODO: break this logic into helpers
Server::Server(int port, const std::string password) : _port(port), _password(password)
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

	// TODO: delete all users

	std::cout << "Server is closed. Bye 👋" << std::endl;
}

void Server::run()
{
	// poll
	while (1)
	{
		poll(&_fds[0], _fds.size(), -1);

		for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
		{
			if (!(it->revents & POLLIN)) // if there's nothing to read, go to next iteration
				continue;

			// case it's server
			if (it->fd == _serverFd)
			{
				// accept client

				struct sockaddr_in addr;
				memset(&addr, 0, sizeof(addr));
				socklen_t addrlen = sizeof(addr);

				pollfd client;
				client.fd = accept4(_serverFd, (struct sockaddr*)&addr, &addrlen, SOCK_NONBLOCK); // accept new client in non-blocking mode
				client.events = POLLIN;
				_fds.push_back(client);
				_users[client.fd] = new User(client.fd, inet_ntoa(addr.sin_addr)); // fd + hostname

				std::cout << "client connected" << std::endl;
			}
			else
			{
				_handleClient(it->fd);
			}

			// case it's client
				// handle client

		}

	}

}
