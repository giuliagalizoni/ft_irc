#ifndef MESSAGE_HPP
# define MESSAGE_HPP

# include <string>
# include <vector>

struct Message {

	std::string              prefix;   // optional, from ":nick!user@host"
	std::string              command;  // e.g. "NICK", "PRIVMSG"
	std::vector<std::string> params;   // all arguments incl. trailing
};

Message parseMessage(std::string line);

#endif
