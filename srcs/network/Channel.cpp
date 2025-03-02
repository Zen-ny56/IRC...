#include "../../include/Channel.hpp"

Channel::Channel(){}

Channel::~Channel(){}

Channel::Channel(const std::string& channelName, const std::string& key, int fd): channelName(channelName), key(key), priOperator(fd), topic(""), inviteOnly(false), max(INT_MAX)
{
	std::string i = "i"; std::string k = "k"; std::string l = "l"; std::string t = "t"; std::string o = "o";
	modes[i] = false;
	modes[k] = false;
	modes[l] = false;
	modes[t] = false;
	modes[o] = false;
	addOperator(fd);
}

Channel& Channel::operator=(const Channel& other)
{
	if (this != &other)
	{
		// Assign each member variable
		// this->channelName = other.channelName; // Note: channelName is const, so cannot be reassigned
		this->key = other.key;                 // key is also const and cannot be reassigned
		this->priOperator = other.priOperator;
		this->topic = other.topic;
		this->inviteOnly = other.inviteOnly;
		this->max = other.max;
		this->modes = other.modes;
		this->clientFds = other.clientFds;
		this->_isInvited = other._isInvited;
		this->_isBanned = other._isBanned;
	}
	return *this;
}

void Channel::addClient(int fd)
{
	clientFds.push_back(fd);
}

void Channel::addOperator(int fd)
{
	operFds.push_back(fd);
	std::string o = "o";
	std::map<std::string, bool>::iterator it = modes.find(o);
	it->second = true;
}

void Channel::removeOperator(int fd)
{
	std::vector<int>::iterator it = std::find(operFds.begin(), operFds.end(), fd);
	if (it != operFds.end())// If foundoper
		operFds.erase(it);    // Remove from the vector
	std::string o = "o";
	std::map<std::string, bool>::iterator bt = modes.find(o);
	bt->second = false;
}

void Channel::setKey(const std::string& key){this->key = key;}

void Channel::setMax(int max){this->max = max;}

int Channel::getMax(){return this->max;}

int Channel::isFull()
{
	int c = 0;
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
		c++;
	if (this->getMax() == c)
		return (1);
	return (0);
}

std::string Channel::getKey(){return this->key;}

int Channel::isInviteOnly()
{
	if (this->inviteOnly == false)
		return (0);
	return (1);
}

int Channel::isInvited(int fd)
{
	if (_isInvited.find(fd) != _isInvited.end())
	{
		if (_isInvited[fd] == false)
			return 1;
	}
	return 0;
}

int Channel::isBanned(const std::string& nickName)
{
	for (std::vector<std::string>::iterator it = _isBanned.begin(); it != _isBanned.end(); ++it)
	{
		if ((*it).compare(nickName) == 0)
			return (1);
	}
	return (0);
}

int Channel::isInChannel(int fd)
{
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		if (*it == fd)
			return (1);
	}
	return (0);
}

void Channel::broadcastToChannel(const std::string& message)
{
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		send(*it, message.c_str(), message.size(), 0);
	}
}

std::string Channel::getTopic(){return this->topic;}

void Channel::setTopic(const std::string& topic){this->topic = topic;}

std::vector<int> Channel::listUsers()
{
	std::vector<int> temp;
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		temp.push_back(*it);
	}
	return(temp);
}

int Channel::getPriOperator(){return this->priOperator;}

std::map<std::string, bool> Channel::getModes(){return this->modes;}

std::string Channel::getChannelName(){return this->channelName;}