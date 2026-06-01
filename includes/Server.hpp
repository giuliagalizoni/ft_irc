#ifndef SERVER_HPP
# define SERVER_HPP

# include "User.hpp"
# include "Channel.hpp"

# include <string>
# include <vector>
# include <map>
# include <poll.h>

class Server
{
	private:
		int _port; // port received from argv
		const std::string _password; // password received from argv
		int _serverFd; // listening socket

		std::vector<struct pollfd> _fds; // poll fds - should include server + users
		std::map<int, User*> _users; // map fd:user
		std::map<std::string, Channel> _channels; // map name:channel

		// Orthodox Canonical Form; private so it's unaccessible
		Server();
		Server(const Server& other);
		Server&	operator=(const Server& other);

		// helper functions to run()
		void _acceptClient(); // accept part
		bool _handleClient(int fd); // recv part

	public:
		Server(int port, const std::string& password);
		~Server();
		void run(); // main function; where poll loop lives

};

#endif
