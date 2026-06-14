#ifndef SETUPEXCEPTION_HPP
# define SETUPEXCEPTION_HPP

# include <exception>
# include <string>

class SetupException : public std::exception
{
	private:
		std::string _msg;

	public:
		explicit SetupException(const std::string& msg);
		~SetupException() throw();
		const char* what() const throw();
};

#endif
