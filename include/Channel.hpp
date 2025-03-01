#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include <algorithm>
#include <cstdio>
#include <set>

class Channel
{
private:
    const std::string channelName;
    std::string key;
    std::string topic;
    bool inviteOnly;
    int max;
    std::vector<int> clientFds;                 // Clients' fds who are presently in the channel
    std::map<int, bool> _isInvited;             // Key = Clients fd; Value = Whether client is Invited in the channel
    std::vector<std::string> _isBanned;         // Vector of nicknames of clients who are banned
    std::set<int> operators;                    // Set of file descriptors of operators
    std::map<int, std::string> clientNicknames; // Map to store client's nickname by fd

public:
    Channel();
    Channel(const std::string &channelName, const std::string &key);
    Channel &operator=(const Channel &copy);
    ~Channel();

    void addClient(int fd);
    void setKey(const std::string &key);
    void setMax(int max);
    bool getInviteOnly();
    std::string getKey();
    int getMax();
    int isFull();
    int isInvited(int fd);
    int isBanned(const std::string &nickName);
    int isInChannel(int fd);
    void broadcastToChannel(const std::string &joinMessage);
    std::string getTopic();
    std::vector<int> listUsers();
    void setTopic(const std::string &topic);
    int isInviteOnly();

    // New functions
    bool isOperator(int fd) const; // Check if the client is an operator
    void addOperator(int fd);      // Add a client as an operator
    void removeOperator(int fd);   // Remove a client from operators
    std::string getOperatorNick() const;
    void removeClient(int fd);
};
