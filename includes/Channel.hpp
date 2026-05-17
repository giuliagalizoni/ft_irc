/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: apaz-mar <apaz-mar@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/17 08:31:21 by apaz-mar          #+#    #+#             */
/*   Updated: 2026/05/17 09:49:28 by apaz-mar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <set>
#include <cstddef> // for size_t

class User;

class Channel
{
    private:
        std::string         _name;
        std::string         _topic;

        std::set<User*>     _users;
        std::set<User*>     _operators;
        std::set<User*>     _invitedUsers;

        bool                _inviteOnly;        // Mode +i
        bool                _topicRestricted;   // Mode +t
        bool                _hasKey;            // Mode +k
        std::string         _key;                
        bool                _hasUserLimit;      // Mode +l
        size_t              _userLimit;          

    public:
        Channel();
        Channel(const std::string& name);
        Channel(const Channel& other);
        Channel& operator=(const Channel& other);
        ~Channel();

        const std::string& getName() const;
        const std::string& getTopic() const;

        void setTopic(const std::string& topic);

        bool hasUser(User* user) const;
        bool isOperator(User* user) const;
        bool isInvited(User* user) const;

        bool addUser(User* user, const std::string& key);
        void removeUser(User* user);

        void addOperator(User* user);
        void removeOperator(User* user);

        void inviteUser(User* user);
        void removeInvitation(User* user);

        void setInviteOnly(bool value);
        void setTopicRestricted(bool value);

        void setKey(const std::string& key);
        void removeKey();

        void setUserLimit(size_t limit);
        void removeUserLimit();

        bool canJoin(User* user, const std::string& key) const;
        bool canChangeTopic(User* user) const;

        const std::set<User*>& getUsers() const;
};

#endif