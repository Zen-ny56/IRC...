#pragma once 

#include "Server.hpp"

class Client //-> class for client
{
private:
	std::string userName;
	std::string realName;
	std::string nickName;
	int fd; //-> client file descriptor
	std::string IPadd; //-> client ip address
	std::map<std::string, bool> faceouthedirt;

public:
	Client();// Default constructor
	int getFd();// Getter for fd
	void setNickname(std::string nickName);
	void setFd(int fd); //-> setter for fd
	std::map<std::string , bool>& getFaceOutheDirt();
	int ifAuthenticated();
	void setIpAdd(std::string ipadd);//-> setter for ipadd
	void setUserName(std::string userName, std::string realName);
	std::string getIPadd();
	std::string getUserName();
	std::string getNickname();

};
