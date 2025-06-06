#pragma once 

#include "Client.hpp"
#include "Server.hpp"

class Client;
class Server;

class Channel
{
	private:
		std::string channelName;
		std::string key;
		std::string topic;
		bool inviteOnly;
		int max;
		bool topicRes;
		std::map<std::string, bool> modeBools;
		std::vector<int> operFds; // Channel operators fds
		std::vector<int> clientFds; //Client's fds of who are presently in the channel	
		std::map<int, bool> _isInvited; // Key = Clients fd; Value = Whether client isInvited in the  channel;
		std::vector<std::string> _isBanned; // Vector of nicknames of clients who are banned
		std::vector<int> _invitationLists; //for every invite adding users to invitation lists
		std::map<std::string, std::string> modes;
	public:
		Channel();
		Channel(const std::string& channelName, const std::string& key, int fd);
		Channel& operator=(const Channel& copy);
		~Channel();
		void remove_isInvited(int fd);
		void addClient(int fd);
		void setKey(const std::string& key);
		void setMax(int max);
		bool getInviteOnly();
		std::string getKey();
		int getMax();
		int isFull();
		int isInvited(int fd);
		bool isInvitedUser(int fd);
		int isBanned(const std::string& nickName);
		int isInChannel(int fd);
		void broadcastToChannel(const std::string& joinMessage);
		void addOperator(int fd);
		std::string getTopic();
		std::vector<int> listUsers();
		void setTopic(const std::string& topic);
		std::map<std::string, bool> & getModes();
		std::vector<int> & getOperFds();
		int isInviteOnly();
		std::string getChannelName();
		void removeOperator(int fd);
		int	 isOperator(int fd);
		void setInviteOnly(bool condition);
		void setTopRes(bool condition);
		bool getTopRes();
		void removeClient(int fd);
		void addToInvitation(int fd);
		void removeClientFromInvitation(int fd);
		int countInchannel();
		void removeModeBools();
        void parseMode(const std::string& message, Client& client); // delete mode after executed
		void reverseRotate(std::stack<std::string>& s); // Reverse rotate on stack in parsing
		void executeMode(Client &client, Server& server);
		std::string generateRPL_CHANNELMODEIS(Client& client, Server& server); // Added
		std::string getFalseParamsAsString(std::stack<std::string> falseParams);
};
