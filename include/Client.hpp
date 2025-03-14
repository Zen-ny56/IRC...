#pragma once 

#include "Server.hpp"

class Client //-> class for client
{
private:
	bool	passAuthen;
	bool	userAuthen;
	bool	nickAuthen;
	std::string userName;
	std::string realName;
	int fd; //-> client file descriptor
	std::string IPadd; //-> client ip address
	std::string nickName;
public:
	Client();// Default constructor
	int getFd();// Getter for fd
	void setNickname(std::string nickName);
	void setFd(int fd); //-> setter for fd
	void setPassAuthen();
	void setUserAuthen();
	void setNickAuthen();
	bool getUserAuthen();
	bool getNickAuthen();
	bool getPassAuthen();
	void setIpAdd(std::string ipadd);//-> setter for ipadd
	void setUserName(std::string userName, std::string realName);
	std::string getIPadd();
	std::string getUserName();
	std::string getNickname();

};
