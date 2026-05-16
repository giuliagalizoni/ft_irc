#include <sys/socket.h> // socket()

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <string>
#include <cstring>

#include "../includes/Server.hpp"

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		return 1; // TODO: exception
	}

	// TODO: parse function
	Server server(std::stoi(argv[1]), argv[2]);
	server.run();
}
// FOR REFERENCE
/*
int	main()
{
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // creating an IPv4 TCP socket with non-blocking mode
	// TODO: double check if the flags are allowed/best practice for 42

	if (fd == -1)
	{
		std::cerr << "Couldn't creat socket" << std::endl;
		return -1;
	}
	int opt = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)  // allow the address/port to be reused sooner
	{
		std::cerr << "setsockopt failed" << std::endl;
		close(fd);
		return -1;
	}

	// binding socket
	struct sockaddr_in addr; // IPv4 socket address structure
	memset(&addr, 0, sizeof(addr)); // initialize struct with zeros
	int port = 6667; // for testing
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(port); // convert the port to network byte order.
	addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any interface

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) // bind socket to address
	{
		std::cerr << "bind failed" << std::endl;
		close(fd);
		return -1;
	}

	if (listen(fd, SOMAXCONN) == -1)// mark the socket as passive (waiting for connections)
	{
		std::cerr << "listen failed" << std::endl;
		close(fd);
		return -1;
	}

	// minimal poll array with 1 client
	struct pollfd fds[2];

	fds[0].fd = fd;  // the server
	fds[0].events = POLLIN; // flag that tells poll() "watch this fd for incoming data"

	fds[1].fd = -1; // one client slot, empty for now
	fds[1].events = POLLIN;

	char buffer[512];

	while (1) // loop to keep it listneing
	{
		poll(fds, 2, -1); // call poll in fds array, size fixed to 2, -1 block until event occurs

		for (int i = 0; i < 2; i++) // for each fd
		{
			if (!(fds[i].revents & POLLIN)) // if there's nothing to read, go to next iteration
				continue;

			if (fds[i].fd == fd) // if it's the server
			{
				socklen_t addrlen = sizeof(addr);
				int client = accept4(fd, (struct sockaddr*)&addr, &addrlen, SOCK_NONBLOCK); // accept new client in non-blocking mode
				fds[1].fd = client; // place the client in fds array, in this case fixed to index 1
				std::cout << "client connected" << std::endl;
			}
			else // if it's the client
			{
				memset(buffer, 0, sizeof(buffer)); // clear the buffer
				int bytes = recv(fds[i].fd, buffer, sizeof(buffer), 0); // read data from socket into buffer
				if (bytes <= 0) // in case recv fails
				{
					std::cout << "client disconnected" << std::endl;
					close(fds[i].fd);
					fds[i].fd = -1;
				}
				else // in case it works
				{
					std::cout << "received:" << buffer << std::endl; // print the message
					std::string reply = "Hello client\r\n";
					send(fds[1].fd, reply.c_str(), reply.size(), 0); // send reply to client
				}
			}
		}
	}
}
*/
