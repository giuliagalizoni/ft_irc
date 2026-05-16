#include "../includes/Server.hpp"

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>

Server::Server(int port, std::string password) : _port(port), _password(password)
{
	// TODO: port should be already validated
	// create listening IPv4 TCP socket with non-blocking mode
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd == -1)
	{
		std::cerr << "Couldn't creat socket" << std::endl;
		//TODO:replace with exception
	}

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)  // allow the address/port to be reused sooner
	{
		std::cerr << "setsockopt failed" << std::endl;
		// TODO: exception; make sure that fd is cleaned up at destruction
	}

	pollfd server_fd;
	server_fd.fd = fd;
	server_fd.events = POLLIN;

	_fds.push_back(server_fd);

	std::cout << "Server started in port: " << _port << std::endl;
}

Server::~Server()
{
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		// iterator behaves like a pointer
		if (it->fd != -1)
			close(it->fd);
	}

		std::cout << "Server is closed. Bye 👋" << std::endl;
}
