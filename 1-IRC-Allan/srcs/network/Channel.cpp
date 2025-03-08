#include "../../include/Channel.hpp"

Channel::Channel() {}

Channel::~Channel() {}

Channel::Channel(const std::string &channelName, const std::string &key) : channelName(channelName), key(key), topic(""), inviteOnly(false), max(INT_MAX) {}

Channel &Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		// Assign each member variable
		// this->channelName = other.channelName; // Note: channelName is const, so cannot be reassigned
		this->key = other.key; // key is also const and cannot be reassigned
		this->topic = other.topic;
		this->inviteOnly = other.inviteOnly;
		this->max = other.max;
		this->clientFds = other.clientFds;
		this->_isInvited = other._isInvited;
		this->_isBanned = other._isBanned;
	}
	return *this;
}

// Channel.cpp (relevant portions)

bool Channel::isOperator(int fd) const
{
	return operators.find(fd) != operators.end();
}

void Channel::addOperator(int fd)
{
	operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
	operators.erase(fd);
}

std::string Channel::getOperatorNick() const
{
	// Iterate over the clientNicknames map
	for (std::map<int, std::string>::const_iterator it = clientNicknames.begin(); it != clientNicknames.end(); ++it)
	{
		int fd = it->first; // File descriptor
		if (isOperator(fd))				   // Check if the client is an operator
			return it->second; // Return the nickname of the operator
	}
	return ""; // Return an empty string if no operator is found
}

void Channel::addClient(int fd)
{
	// Step 1: Add the client to the list of clients in the channel
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		if (*it == fd)
		{
			std::string errMessage = std::string(RED) + " User already exist in channel " + std::string(EN);
			int bytes =  send(fd, errMessage.c_str(), errMessage.size(), 0);
			if (bytes == -1)
				std::cerr << "Error: messages could not be sent." << std::endl;
			return ;
		}
	}
	clientFds.push_back(fd);
	// Step 2: If it's the first client, assign them as the operator
	if (clientFds.size() == 1)
		addOperator(fd); // Automatically make the first client an operator
}

void Channel::setKey(const std::string &key) { this->key = key; }

void Channel::setMax(int max) { this->max = max; }

int Channel::getMax() { return this->max; }

int Channel::isFull()
{
	int c = 0;
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
		c++;
	if (this->getMax() == c)
		return (1);
	return (0);
}

std::string Channel::getKey() { return this->key; }

int Channel::isInviteOnly()
{
	if (this->inviteOnly == false)
		return (0);
	return (1);
}

void Channel::removeClient(int fd)
{
	std::cout << "Attempting to remove client with fd: " << fd << std::endl;

	// Find the fd in the vector
	std::vector<int>::iterator it = std::find(clientFds.begin(), clientFds.end(), fd);

	// If the client is found, remove them
	if (it != clientFds.end())
	{
		std::cout << "Client with fd " << fd << " found. Removing." << std::endl;
		clientFds.erase(it); // Erase the found fd
		return;
	}

	// If the client with the given fd is not found
	std::cerr << "Client with fd " << fd << " not found in clientFds vector." << std::endl;
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

int Channel::isBanned(const std::string &nickName)
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

void Channel::broadcastToChannel(const std::string &message)
{
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		send(*it, message.c_str(), message.size(), 0);
	}
}

std::string Channel::getTopic() { return this->topic; }

void Channel::setTopic(const std::string &topic) { this->topic = topic; }

std::vector<int> Channel::listUsers()
{
	std::vector<int> temp;
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		temp.push_back(*it);
	}
	return (temp);
}
