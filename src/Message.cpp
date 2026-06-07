#include "../includes/Message.hpp"

#include <cctype>

static void getPrefix(Message& msg, std::string& line)
{
	std::size_t found = line.find(" "); // find the first space;
	if (line[0] == ':') // chekc if starts with prefix :
	{
		if (found != std::string::npos) // if there's a space
		{
			msg.prefix = line.substr(1, found - 1); // extract the prefix from after : to before space
			line.erase(0, found + 1); // clean the extracted part from line
		}
		else // if there's no space
		{
			//TODO: exception
			return;
		}
	}
}

static void getCommand(Message& msg, std::string& line)
{
	std::size_t found = line.find(" "); // find the next space
	if (found != std::string::npos) // if there's a space
	{
		msg.command = line.substr(0, found); // extract the command up to the space
		line.erase(0, found + 1); // clean the extracted part from line
	}
	else // no space
		msg.command = line; // take all the line

	for(int i = 0; i < msg.command.size(); i++) // covert it to upper case to be sure
		 msg.command[i] = toupper(msg.command[i]);

}

static void getParams(Message& msg, std::string& line)
{
	while(!line.empty()) // while there's still something remaining in line
	{
		if (line[0] == ':') // if it's starts with :
		{
			msg.params.push_back(line.substr(1)); // add everything from after the :
			break; // and exit the loop
		}
		std::size_t found = line.find(" "); // find the next space
		if (found != std::string::npos) // it there's one
		{
			if (found != 0)
				msg.params.push_back(line.substr(0, found)); // push everything up to the space to params vector
			line.erase(0, found + 1); // clean the extracted part
		}
		else // if there's no space and line is not empty, it's the last parameter
		{
			msg.params.push_back(line); // push everything in line
			break; // exit the loop
		}
	}
}

Message parseMessage(std::string line)
{
	// TODO
	// if (line.empty())
	// {
	// 	throw exception
	// }

	Message msg;

	getPrefix(msg, line);
	getCommand(msg, line);
	getParams(msg,line);

	return msg;
}
