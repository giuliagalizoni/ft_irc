#include "../includes/Server.hpp"
#include "../includes/signals.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <exception>

// Parses and validates the port argument from argv; returns the port number or -1 on error.
int validPort(char *str)
{
	if (str[0] == '+')
		return -1;
	std::istringstream iss(str);
	long port;
	iss >> port;

	if (iss.fail() || !iss.eof())
		return -1;
	if (port <= 0 || port > 65535)
			return -1;

	return port;
}

// Returns true if the password is non-empty and contains no whitespace or control characters.
bool validPass(const std::string& pass)
{
	if (pass.empty())
		return false;
	for (std::string::size_type i = 0; i < pass.size(); i++)
	{
		if (isspace(static_cast<unsigned char>(pass[i])))
			return false;
		if (iscntrl(static_cast<unsigned char>(pass[i])))
			return false;
	}
	return true;
}

// Entry point: validates args, sets up signals, and runs the server.
int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	int port = validPort(argv[1]);
	if (port < 0)
	{
		std::cerr << "Invalid port" << std::endl;
		return 1;
	}

	if (!validPass(argv[2]))
	{
		std::cerr << "Invalid password" << std::endl;
		return 1;
	}

	try
	{
		setupSignals();
		Server server(port, argv[2]);
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
