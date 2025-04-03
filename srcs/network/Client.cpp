#include "../../include/Client.hpp"

Client::Client() :userName("default"), realName("default"), nickName("default")
{
    this->faceouthedirt[std::string("nick")] = false;
	this->faceouthedirt[std::string("user")] = false;
	this->faceouthedirt[std::string("pass")] = false;
}

int Client::getFd(){return fd;}

void Client::setFd(int fd){this->fd = fd;}

void Client::setIpAdd(std::string IPadd){this->IPadd = IPadd;}

std::string Client::getIPadd(){return this->IPadd;}

int Client::ifAuthenticated()
{
    for (std::map<std::string, bool>::iterator it = faceouthedirt.begin(); it != faceouthedirt.end(); ++it)
    {
        if (it->second == false)
            return 1;
    }
    return 0;
}
std::map<std::string , bool>& Client::getFaceOutheDirt()
{
    return this->faceouthedirt;
}

std::string Client::getNickname(){return this->nickName;}

std::string Client::getUserName(){return this->userName;}

void Client::setNickname(std::string nickName)
{
    this->nickName = nickName;
    std::map<std::string, bool>::iterator it = faceouthedirt.find("nick");
    it->second = true;
}

void Client::setUserName(std::string userName, std::string realName)
{
    this->userName = userName;
    this->realName = realName;
    std::map<std::string, bool>::iterator it = faceouthedirt.find("user");
    it->second = true;
}

void Client::setNeedsCap(bool condition)
{
    this->needsCap = condition;
}

bool Client::getNeedsCap()
{
    return this->needsCap;
}

void Client::setCapisSuccess(bool condition)
{
    this->capisSuccess = condition;
}

bool Client::getCapisSuccess()
{
    return this->capisSuccess;
}