#pragma once

#include <iostream>
#include <sstream>
#include <vector>       //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h>  //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h>      //-> for fcntl()
#include <unistd.h>     //-> for close()
#include <arpa/inet.h>  //-> for inet_ntoa()
#include <poll.h>       //-> for poll()
#include <csignal>      //-> for signal()
#include <map>
#include <cstdlib>    //-> for std::atoi()
#include <cstring>    //-> for memset()
#include <climits>    //-> for INT_MAX
#include "Client.hpp" //-> for the class Client
#include "Channel.hpp"
//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color
#define EN "\0m"       // -> for closing color
class Client;
class Channel;

class Server //-> class for server
{
private:
    int serSocketFd;    //-> server socket file descriptor
    static bool signal; //-> static boolean for signal
    int port;           //-> server port
    std::string password;
    std::vector<Client> clients;             //-> vector of Clients
    std::vector<struct pollfd> fds;          //-> vector of pollfd
    std::map<std::string, int> nicknameMap;  //-> map for nickname check
    std::map<std::string, Channel> channels; // ->map of Channels

public:
    Server();                                    //-> default constructor
    void serverInit(int port, std::string pass); //-> server initialization
    void serSocket();                            //-> server socket creation
    void acceptNewClient();                      //-> accept new client
    void receiveNewData(int fd);                 //-> receive new data from a registered client
    static void signalHandler(int signum);       //-> signal handler
    void closeFds();                             //-> close file descriptors
    void clearClients(int fd);                   //-> clear clients
    void sendCapabilities(int fd);
    void processCapReq(int fd, const std::string &message);
    void markPasswordAccepted(int fd);
    void validatePassword(int fd, const std::string &message);
    void processNickUser(int fd, const std::string &message);
    void processSasl(int fd, const std::string &message);
    bool isValidNickname(const std::string &nickname);
    void processUser(int fd, const std::string &message);
    void capEnd(int fd);
    std::vector<Client>::iterator getClient(int fd);
    void handleChannel(int fd, const std::string &message);
    void processQuit(int fd, const std::string &reason);
    void disconnectClient(int fd);
    void joinChannel(int fd, const std::string &channelName, const std::string &key);
    std::vector<std::string> splitByDelimiter(const std::string &str, char delimiter);
    bool isValidChannelName(const std::string &channelName);
    std::map<std::string, Channel>::iterator isValidChannelName(std::string &channelName);
    void sendWelcome(int fd, Client &client);
    Client &operator[](std::vector<Client>::iterator it);
    void processPrivmsg(int fd, const std::string &message);
    std::vector<Client>::iterator getClientUsingNickname(const std::string &nickname);
    std::string trim(const std::string &str);
    // new functions
    void kickCommand(int fd, const std::string &message);
    void topicCommand(int fd, const std::string &message);
};
