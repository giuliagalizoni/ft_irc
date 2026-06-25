#ifndef USER_HPP
#define USER_HPP

#include <string>

class User
{
	private:
		int _fd;
		std::string _buffer;
		std::string _outBuffer;
		std::string _hostname; // from accept
		std::string _nickname;
		std::string _username;
		std::string _realname; // from USER command
		// registration tracking
		bool _passReceived;
		bool _nickReceived;
		bool _userReceived;

		bool _registrationAnnounced; // true after 001 is sent; prevents re-announcing on nick change
		bool _disconnecting; // to handle disconection without shutting down the server


		User();
		User(const User& other);
		User& operator=(const User& other);

	public:
		User(int fd, const std::string& hostname);
		~User();


		void appendToBuffer(const std::string& data); // appends raw bytes to _buffer
		bool getNextLine(std::string& line); // returns false if no complete line available

		void sendMessage(const std::string& msg);
		bool hasPendingOutput() const;
		void flushOutput();

		int getFd() const;
		bool isRegistered() const; // true when all three flags are set


		std::string getNickname() const;
		std::string getUsername() const;
		std::string getRealname() const;
		std::string getHostname() const;

		void setNickname(const std::string& nickname);
		void setUsername(const std::string& username);
		void setRealname(const std::string& realname);

		void setPassReceived();
		void setNickReceived();
		void setUserReceived();

		bool isRegistrationAnnounced();
		void setRegistrationAnnounced();

		void setDisconnecting();
		bool getDisconnecting() const;
};

#endif
