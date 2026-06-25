#include "../includes/Channel.hpp"

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

Channel::Channel(const Channel& other)
{
	*this = other;
}

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

Channel::~Channel()
{}

const std::string& Channel::getName() const { return (_name); }
const std::string& Channel::getTopic() const { return (_topic); }

void Channel::setTopic(const std::string& topic)
{
	_topic = topic;
}

bool Channel::hasUser(User* user) const
{
	return (_users.find(user) != _users.end());
}

bool Channel::isOperator(User* user) const
{
	return (_operators.find(user) != _operators.end());
}

bool Channel::isInvited(User* user) const
{
	return (_invitedUsers.find(user) != _invitedUsers.end());
}

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

bool Channel::addUser(User* user, const std::string& key)
{
	if (canJoin(user, key) != JOIN_OK)
		return (false);
	_users.insert(user);
	_invitedUsers.erase(user);      //canJoin already checks if there is invitation, and erase doesnt crash erasing a non-existent user

	if (_users.size() == 1)         // first user entering an empty channel becomes channel operator
		_operators.insert(user);
	return (true);
}

void Channel::removeUser(User* user)
{
	_users.erase(user);
	_operators.erase(user);
	_invitedUsers.erase(user);
}

void Channel::addOperator(User* user)
{
	if (hasUser(user))
		_operators.insert(user);
}

void Channel::removeOperator(User* user)
{
	_operators.erase(user);
}

void Channel::inviteUser(User* user)
{
	if (user)
		_invitedUsers.insert(user);
}

void Channel::removeInvitation(User* user)
{
	_invitedUsers.erase(user);
}

void Channel::setInviteOnly(bool value)
{
	_inviteOnly = value;
}

void Channel::setTopicRestricted(bool value)
{
	_topicRestricted = value;
}

void Channel::setKey(const std::string& key)
{
	_hasKey = true;
	_key = key;
}

void Channel::removeKey()
{
	_hasKey = false;
	_key.clear();
}

void Channel::setUserLimit(size_t limit)
{
	_hasUserLimit = true;
	_userLimit = limit;
}

void Channel::removeUserLimit()
{
	_hasUserLimit = false;
	_userLimit = 0;          //just for cleanup
}

bool Channel::canChangeTopic(User* user) const // just checks if this user is allowed to change the topic
{
	if (!hasUser(user))
		return (false);
	if (_topicRestricted && !isOperator(user))
		return (false);
	return (true);
}

const std::set<User*>& Channel::getUsers() const
{
	return (_users);
}

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

