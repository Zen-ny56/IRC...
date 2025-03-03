#pragma once 

#include "Client.hpp"
#include "Server.hpp"

class Channel
{
	private:
		// std::map<char, bool> modes;
		std::string channelName;
		std::string key;
		int priOperator;
		std::string topic;
		bool inviteOnly;
		int max;
		std::map<std::string, bool> modes;
		std::vector<int> operFds; // Channel operators fds
		std::vector<int> clientFds; //Client's fds of who are presently in the channel	
		std::map<int, bool> _isInvited; // Key = Clients fd; Value = Whether client isInvited in the  channel;
		std::vector<std::string> _isBanned; // Vector of nicknames of clients who are banned
	public:
		Channel();
		Channel(const std::string& channelName, const std::string& key, int fd);
		Channel& operator=(const Channel& copy);
		~Channel();
		int getPriOperator();
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
		void addOperator(int fd);
		std::string getTopic();
		std::vector<int> listUsers();
		void setTopic(const std::string& topic);
		std::map<std::string, bool> & getModes();
		int isInviteOnly();
		std::string getChannelName();
		void removeOperator(int fd);
		int	 isOperator(int fd);
};
