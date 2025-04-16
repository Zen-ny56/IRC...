#include "../../include/Channel.hpp"

Channel::Channel(){}

Channel::~Channel(){}

//Changed constructor
Channel::Channel(const std::string& channelName, const std::string& key, int fd): channelName(channelName), key(key), topic(""), inviteOnly(false), max(INT_MAX), topicRes(false)
{
	std::string i = "i"; std::string k = "k"; std::string l = "l"; std::string t = "t"; std::string o = "o";
	modeBools[i] = false;
	if (!key.empty())
		modeBools[k] = true;
	else
		modeBools[k] = false;
	modeBools[l] = false;
	modeBools[t] = false;
	modeBools[o] = false;
	addOperator(fd);
}

//Changed copy assignment operator
Channel& Channel::operator=(const Channel& other)
{
	if (this != &other)
	{
		// Assign each member variable
		this->channelName = other.channelName; // Note: channelName is const, so cannot be reassigned
		this->key = other.key;                 // key is also const and cannot be reassigned
		this->topic = other.topic;
		this->inviteOnly = other.inviteOnly;
		this->max = other.max;
		this->topicRes = other.topicRes;
		this->modeBools = other.modeBools;
		this->operFds = other.operFds;
		this->clientFds = other.clientFds;
		this->_isInvited = other._isInvited;
		this->_isBanned = other._isBanned;
		this->modes = other.modes;
	}
	return *this;
}

void Channel::addClient(int fd)
{
	clientFds.push_back(fd);
}

int Channel::countInchannel(){
	return clientFds.size();
}

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

void Channel::addToInvitation(int fd){
	_invitationLists.push_back(fd);
}

bool Channel::isInvitedUser(int fd)
{
	for (std::vector<int>::iterator it = _invitationLists.begin(); it != _invitationLists.end(); ++it)
	{
		if (*it == fd)
			return true;
	}
	return false;
}

void Channel::removeClientFromInvitation(int fd)
{
    for (std::vector<int>::iterator it = _invitationLists.begin(); it != _invitationLists.end(); ++it)
    {
        if (*it == fd)
        {
            _invitationLists.erase(it);
            break;
        }
    }
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
	if (clientFds.begin() == clientFds.end())
		return(temp);
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		temp.push_back(*it);
	}
	return(temp);
}

//Get Modes
std::map<std::string, bool> & Channel::getModes(){return this->modeBools;}

//Get this
std::string Channel::getChannelName(){ 	std::cout << channelName << std::endl;
return this->channelName;}

//Is operator
int Channel::isOperator(int fd)
{
	for (std::vector<int>::iterator it = operFds.begin(); it != operFds.end(); ++it)
	{
		if (*it == fd)
			return (1);
	}
	return (0);
}

std::vector<int> & Channel::getOperFds(){return this->operFds;}

void Channel::setInviteOnly(bool condition){ this->inviteOnly = condition;}

//Topic Restrictions 
void Channel::setTopRes(bool condition){ this->topicRes = condition;}

bool Channel::getTopRes(){ return this->topicRes;}

// Remove and add Operator
void Channel::addOperator(int fd)
{
	operFds.push_back(fd);
}

void Channel::removeOperator(int fd)
{
	for (std::vector<int>::iterator it = operFds.begin(); it != operFds.end(); ++it)
	{
		if (*it == fd)
		{	operFds.erase(it); break;}
		
	}
}

//Key function
void Channel::setKey(const std::string& key){this->key = key;}
std::string Channel::getKey(){return this->key;}

void Channel::removeClient(int fd)
{
	for (std::vector<int>::iterator it = clientFds.begin(); it != clientFds.end(); ++it)
	{
		if (*it == fd)
		{ 
			if(isOperator(*it))
				removeOperator(*it);
			clientFds.erase(it); 
			break;
		}
	}
}

void Channel::remove_isInvited(int fd)
{
	std::map<int, bool>::iterator it = _isInvited.find(fd);
	if (it == _isInvited.end())
		return;
	_isInvited.erase(it->first);
}

void Channel::removeModeBools()
{
	for (std::map<std::string, bool>::iterator it = modeBools.begin(); it != modeBools.end(); ++it)
	{
		modeBools.erase(it->first);
	}
}

void Channel::reverseRotate(std::stack<std::string> &s)
{
	if (s.empty() || s.size() == 1)
		return; // Nothing to rotate if stack has 0 or 1 element
	std::queue<std::string> tempQueue;
	// Step 1: Move all elements except the last one to a queue
	while (s.size() > 1)
	{
		tempQueue.push(s.top());
		s.pop();
	}
	// Step 2: The last remaining element is the bottom-most element
	std::string bottomElement = s.top();
	s.pop();
	// Step 3: Restore the elements back to the stack in original order
	while (!tempQueue.empty())
	{
		s.push(tempQueue.front());
		tempQueue.pop();
	}
	// Step 4: Push the bottom-most element to the top
	s.push(bottomElement);
}

std::string Channel::getFalseParamsAsString(std::stack<std::string> falseParams)
{
	std::string result;
	size_t count = falseParams.size();

	for (size_t i = 0; i < count; ++i)
	{
		reverseRotate(falseParams);
		// Get the top (original bottom-most) element
		result += falseParams.top();
		falseParams.pop();

		if (i < count - 1)
			result += " ";
	}

	return result;
}