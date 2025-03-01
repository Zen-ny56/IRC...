#pragma once 

#include "Client.hpp"
#include "Server.hpp"

class Channel
{
	private:
		// std::map<char, bool> modes;
		const std::string channelName;
		std::string key;
		std::string topic;
		bool inviteOnly;
		int max;
		std::vector<int> clientFds; //Client's fds of who are presently in the channel	
		std::map<int, bool> _isInvited; // Key = Clients fd; Value = Whether client isInvited in the  channel;
		std::vector<std::string> _isBanned; // Vector of nicknames of clients who are banned
	public:
		Channel();
		Channel(const std::string& channelName, const std::string& key);
		Channel& operator=(const Channel& copy);
		~Channel();
		void addClient(int fd);
		void setKey(const std::string& key);
		void setMax(int max);
		bool getInviteOnly();
		std::string getKey();
		int getMax();
		int isFull();
		int isInvited(int fd);
		int isBanned(const std::string& nickName);
		int isInChannel(int fd);
		void broadcastToChannel(const std::string& joinMessage);
		std::string getTopic();
		std::vector<int> listUsers();
		void setTopic(const std::string& topic);
		int isInviteOnly();
};
