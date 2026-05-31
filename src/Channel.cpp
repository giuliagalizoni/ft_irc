/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: apaz-mar <apaz-mar@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 17:46:31 by apaz-mar          #+#    #+#             */
/*   Updated: 2026/05/31 18:45:56 by apaz-mar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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



