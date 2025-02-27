#include "../../include/Client.hpp"

Client::Client() :passAuthen(false), userAuthen(false), nickAuthen(false), userName("default"), realName("default"){}

int Client::getFd(){return fd;}

void Client::setFd(int fd){this->fd = fd;}

void Client::setIpAdd(std::string IPadd){this->IPadd = IPadd;}

void Client::setNickname(std::string nickName)
{
    this->nickName = nickName;
    this->nickAuthen = true;
}

void Client::setUserName(std::string userName, std::string realName)
{
    this->userName = userName;
    this->realName = realName;
    this->userAuthen = true;
}

std::string Client::getIPadd(){return this->IPadd;}

void Client::setPassAuthen(){this->passAuthen = true;}

bool Client::getUserAuthen(){return this->userAuthen;}

bool Client::getNickAuthen(){return this->nickAuthen;}

bool Client::getPassAuthen(){return this->passAuthen;}

std::string Client::getNickname(){return this->nickName;}

std::string Client::getUserName(){return this->userName;}