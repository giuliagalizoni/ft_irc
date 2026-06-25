#ifndef SERVER_HPP
#define SERVER_HPP

#include "User.hpp"
#include "Channel.hpp"
#include "Command.hpp"

#include <string>
#include <vector>
#include <map>
#include <poll.h>

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
		void _handleClient(int fd); // recv part

		bool _processCommand(int fd, std::string& line);
		void _handleJoin(int fd, const Command& cmd);
		void _handleNick(int fd, const Command& cmd);
		void _broadcastToChannel(Channel& channel, const std::string& msg, int exceptFd);

		Command _parseCommand(std::string& line);
		void getCommand(Command& cmd, std::string& line);
		void getParams(Command& cmd, std::string& line);

		void _handlePass(int fd, const Command& cmd);
		void _handleUser(int fd, const Command& cmd);

		void _checkRegistration(int fd);

		bool _nicknameExists(const std::string& nickname, int exceptFd) const;

		void _handlePrivmsg(int fd, const Command& cmd);
		User* _findUserByNickname(const std::string& nickname) const;

		void _handlePart(int fd, const Command& cmd);
		bool _handleQuit(int fd, const Command& cmd);

		void _handleTopic(int fd, const Command& cmd);
		void _handleInvite(int fd, const Command& cmd);
		void _handleKick(int fd, const Command& cmd);

		void _handleMode(int fd, const Command& cmd);
		void _disconnectClient(int fd);

		std::string _userPrefix(User* user) const;
		void _sendNumeric(int fd, const std::string& code, const std::string& text);
		void _handlePing(int fd, const Command& cmd);
		void _handleWhois(int fd, const Command& cmd);

	public:
		Server(int port, const std::string& password);
		~Server();
		void run(); // main function; where poll loop lives
};

#endif
