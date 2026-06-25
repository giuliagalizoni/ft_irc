#include "../includes/Channel.hpp"

// Default constructor: initialises an empty channel with all modes off.
Channel::Channel()
	:   _name(""),
		_topic(""),
		_inviteOnly(false),
		_topicRestricted(false),
		_hasKey(false),
		_key(""),
		_hasUserLimit(false),
		_userLimit(0)
{}

// Named constructor: same as default but sets the channel name.
Channel::Channel(const std::string& name)
	:   _name(name),
		_topic(""),
		_inviteOnly(false),
		_topicRestricted(false),
		_hasKey(false),
		_key(""),
		_hasUserLimit(false),
		_userLimit(0)
{}

// Copy constructor.
Channel::Channel(const Channel& other)
{
	*this = other;
}

// Copy assignment operator.
Channel& Channel::operator=(const Channel& other)
{
	if (this != &other)
	{
		_name = other._name;
		_topic = other._topic;
		_users = other._users;
		_operators = other._operators;
		_invitedUsers = other._invitedUsers;
		_inviteOnly = other._inviteOnly;
		_topicRestricted = other._topicRestricted;
		_hasKey = other._hasKey;
		_key = other._key;
		_hasUserLimit = other._hasUserLimit;
		_userLimit = other._userLimit;
	}
	return (*this);
}

// Destructor.
Channel::~Channel()
{}

// Returns the channel name.
const std::string& Channel::getName() const { return (_name); }

// Returns the current topic.
const std::string& Channel::getTopic() const { return (_topic); }

// Sets the channel topic.
void Channel::setTopic(const std::string& topic)
{
	_topic = topic;
}

// Returns true if the user is in the channel.
bool Channel::hasUser(User* user) const
{
	return (_users.find(user) != _users.end());
}

// Returns true if the user has operator status.
bool Channel::isOperator(User* user) const
{
	return (_operators.find(user) != _operators.end());
}

// Returns true if the user is on the invite list.
bool Channel::isInvited(User* user) const
{
	return (_invitedUsers.find(user) != _invitedUsers.end());
}

// Returns the reason a join would succeed or fail (invite-only, wrong key, user limit, etc.).
Channel::JoinResult Channel::canJoin(User* user, const std::string& key) const
{
	if (!user || hasUser(user))
		return (JOIN_ALREADY);
	if (_inviteOnly && !isInvited(user))
		return (JOIN_INVITEONLY);
	if (_hasKey && key != _key)
		return (JOIN_BADKEY);
	if (_hasUserLimit && _users.size() >= _userLimit)
		return (JOIN_FULL);
	return (JOIN_OK);
}

// Adds the user to the channel if canJoin passes; the first user automatically becomes operator.
bool Channel::addUser(User* user, const std::string& key)
{
	if (canJoin(user, key) != JOIN_OK)
		return (false);
	_users.insert(user);
	_invitedUsers.erase(user);

	if (_users.size() == 1)
		_operators.insert(user);
	return (true);
}

// Removes the user from the channel, operator list, and invite list.
void Channel::removeUser(User* user)
{
	_users.erase(user);
	_operators.erase(user);
	_invitedUsers.erase(user);
}

// Grants operator status to a user already in the channel.
void Channel::addOperator(User* user)
{
	if (hasUser(user))
		_operators.insert(user);
}

// Revokes operator status.
void Channel::removeOperator(User* user)
{
	_operators.erase(user);
}

// Adds a user to the invite list.
void Channel::inviteUser(User* user)
{
	if (user)
		_invitedUsers.insert(user);
}

// Removes a user from the invite list.
void Channel::removeInvitation(User* user)
{
	_invitedUsers.erase(user);
}

// Sets or clears invite-only mode (+i).
void Channel::setInviteOnly(bool value)
{
	_inviteOnly = value;
}

// Sets or clears topic-restricted mode (+t).
void Channel::setTopicRestricted(bool value)
{
	_topicRestricted = value;
}

// Sets the channel key and enables mode +k.
void Channel::setKey(const std::string& key)
{
	_hasKey = true;
	_key = key;
}

// Clears the channel key and disables mode +k.
void Channel::removeKey()
{
	_hasKey = false;
	_key.clear();
}

// Sets the user limit and enables mode +l.
void Channel::setUserLimit(size_t limit)
{
	_hasUserLimit = true;
	_userLimit = limit;
}

// Removes the user limit and disables mode +l.
void Channel::removeUserLimit()
{
	_hasUserLimit = false;
	_userLimit = 0;
}

// Returns true if the user may change the topic (must be in channel; operator required if +t is set).
bool Channel::canChangeTopic(User* user) const
{
	if (!hasUser(user))
		return (false);
	if (_topicRestricted && !isOperator(user))
		return (false);
	return (true);
}

// Returns the set of all users currently in the channel.
const std::set<User*>& Channel::getUsers() const
{
	return (_users);
}

// Returns the active mode flags as a string (e.g. "+ikt"); returns empty string if no modes are set.
std::string Channel::getModeString() const
{
	std::string modes = "+";

	if (_inviteOnly)
		modes += "i";
	if (_topicRestricted)
		modes += "t";
	if (_hasKey)
		modes += "k";
	if (_hasUserLimit)
		modes += "l";

	if (modes == "+")
		return "";

	return modes;
}
