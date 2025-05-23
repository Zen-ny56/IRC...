#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include "../../include/Channel.hpp"

void Channel::parseMode(const std::string &message, Client& client)
{
	std::istringstream iss(message);
	std::string command, channelName, token;
	std::map<std::string, std::string> modeMap; // Store mode-parameter pairs
	std::stack<std::string> modeWitPams;
	std::stack<std::string> params; // Keep track of order of modes needing parameters
	std::stack<std::string> falseParams;
	bool orderAfterOperator = false;
	int k = 0;

	// Read command
	iss >> command;
	iss >> channelName;
	// Process input
	std::string type;
	while (iss >> token)
	{
		std::cout << token << std::endl;
		if (!token.empty() && (token[0] == '+' || token[0] == '-'))
		{
			// New mode group detected, process each mode separately
			for (size_t i = 1; i < token.size(); ++i)
			{
				if (token[i] == '+' && token[0] == '+') // Ignore duplicates
					continue;
				if ((token[i] == '-' && token[i - 1] == '+') || (token[i] == '+' && token[i - 1] == '-')) // Ignore alternating "+-" patterns (e.g., "+-i" or "-+o")
				{
					continue; // Skip ineffective mode
				}
				if (token[i] == 'i' || token[i] == 't') // If mode is 'i' or 't', it never takes a parameter
				{
					type = std::string(1, token[0]) + token[i];
					modeMap[type] = ""; // Ensure empty parameter
				}
				else if (token[i] == 'l' || token[i] == 'o' || token[i] == 'k')
				{
					orderAfterOperator = true;
					if (token[0] == '-' && token[i] == 'k')
					{

						type = std::string(1, token[0]) + token[i];
						modeMap[type] = "";
					}
					else
					{
						type = std::string(1, token[0]) + token[i];
						modeWitPams.push(type);
						k++;
					}
				}
				else
				{
					type = std::string(1, token[0]) + token[i];
					modeMap[type] = "";
				}
			}
		}
		else if (!token.empty() && orderAfterOperator == true)
		{
			type = token;
			params.push(type);
		}
		else
		{
			type = token;
			falseParams.push(type);
		}
	}
	int j = k;
	while (k-- > 1)
		reverseRotate(modeWitPams);
	while (j-- > 1)
		reverseRotate(params);
	while (!modeWitPams.empty() && !params.empty())
	{
		std::string bojo = modeWitPams.top();
		modeWitPams.pop();
		std::string yoyo = params.top();
		params.pop();
		modeMap[bojo] = yoyo;
	}
	if (!params.empty())
	{
		std::string extraf = params.top();
		params.pop();
		falseParams.push(extraf);
	}
	if (!modeWitPams.empty())
	{
		std::string emptyPam = modeWitPams.top();
		modeWitPams.pop();
		modeMap[emptyPam] = "";
	}
	std::string warning = getFalseParamsAsString(falseParams);
	if (!warning.empty())
	{
    	std::string msg =  std::string(EN) + "Warning: extra parameters detected: " + warning + "\r\n";
		send(client.getFd(), msg.c_str(), msg.size(), 0);
	}
	this->modes = modeMap;
}

void Server::handleMode(int fd, const std::string &message)
{
	std::istringstream iss(message);
	std::string command, channelName;
	iss >> command;
	iss >> channelName;
	std::vector<Client>::iterator it = getClient(fd);
	if (it == clients.end())
		throw std::runtime_error("Client was not found]\n");
	Client &client = (*this)[it];
	std::map<std::string, Channel>::iterator bt = channels.find(channelName);
	if (bt == channels.end())
	{
		std::string msg = std::string(RED) + ":" + this->hostname + " 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n" + std::string(EN);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
	}
	Channel &channel = bt->second;
	if (!channel.isOperator(fd))
	{
		std::string msg = std::string(RED) + ":" + this->hostname + " 482 " + client.getNickname() + " " + channel.getChannelName() + " :You're not channel operator\r\n" + std::string(EN);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
	}
	channel.parseMode(message, client);
	channel.executeMode(client, *this);
	channel.generateRPL_CHANNELMODEIS(client, *this);
}

void Channel::executeMode(Client &client, Server& server)
{
	for (std::map<std::string, std::string>::iterator ct = modes.begin(); ct != modes.end(); ++ct)
	{
		std::string i = "i";
		std::string k = "k";
		std::string l = "l";
		std::string t = "t";
		std::string o = "o";
		std::string modeStr = ct->first; // Example: "+l"
		std::cout << ct->second << std::endl;
		std::string msg;
		int limit;
		char modeType = modeStr[1];
		std::vector<Client>::iterator targetIt;
		std::map<std::string, bool>::iterator it;
		std::vector<int>::iterator dt;
		if (modeStr[0] == '+')
		{
			switch (modeType)
			{
			case 'l':
				// Check if it's valid integer and convert
				if (isNumber(ct->second) == false)
				{
					msg = std::string(RED) + ":ircserv 461 " + client.getNickname() + " " + this->getChannelName() + " :Not enough parameters" + std::string(EN);
					send(client.getFd(), msg.c_str(), msg.size(), 0);
					return;
				}
				limit = stringToInt(ct->second);
				if (limit <= this->countInchannel())
				{
					std::ostringstream oss;
					oss << limit;
					std::string errorMsg = std::string(RED) + ":" + server.getHostname() + " 471 " + client.getNickname() + " " + getChannelName() + " :(+l) input size " + oss.str() + " is less or equal to clients in channel\r\n" + std::string(EN);
					send(client.getFd(), errorMsg.c_str(), errorMsg.size(), 0);
					return;
				}
				this->setMax(limit);
				it = modeBools.find(l);
				it->second = true;
				break;
			case 'k':
				setKey(ct->second);
				it = modeBools.find(k);
				it->second = true;
				break;
			case 't':
				setTopRes(true);
				it = modeBools.find(t);
				it->second = true;
				break;
			case 'i':
				setInviteOnly(true);
				it = modeBools.find(i);
				it->second = true;
				break;
			case 'o':
				targetIt = server.getClientUsingNickname(ct->second);
				for (dt = clientFds.begin(); dt != clientFds.end(); ++dt)
				{
					if (*dt == targetIt->getFd())
						break;
				}
				if (dt == clientFds.end())
				{
					msg = std::string(RED) + ":" + server.getHostname() + " 401 " + client.getNickname() + " " + ct->second + " :No such nick/channel\r\n" + std::string(EN);
					send(client.getFd(), msg.c_str(), msg.size(), 0); // ERR_NOSUCHNICK
				}
				addOperator(targetIt->getFd());
				it = modeBools.find(o);
				it->second = true;
				break;
			default:
				msg = std::string(RED) + ":" + server.getHostname() + " 472 " + client.getNickname() + " " + ct->first + " :is unknown mode char to me\r\n" + std::string(EN);
				send(client.getFd(), msg.c_str(), msg.size(), 0); // ERR_ MODE NOT AVAILABLE
				break;
			}
		}
		else
		{
			switch (modeType)
			{
			case 'l':
				// Check if it's valid integer and convert
				setMax(INT_MAX);
				it = modeBools.find(l);
				it->second = false;
				break;
			case 'k':
				it = modeBools.find(k);
				it->second = false;
				break;
			case 't':
				setTopRes(false);
				it = modeBools.find(t);
				it->second = false;
				break;
			case 'i':
				setInviteOnly(false);
				it = modeBools.find(i);
				it->second = false;
				break;
			case 'o':
				targetIt = server.getClientUsingNickname(ct->second);
				for (dt = clientFds.begin(); dt != clientFds.end(); ++dt)
				{
					if (*dt == targetIt->getFd())
						break;
				}
				if (dt == clientFds.end())
				{
					msg = std::string(RED) + ":" + server.getHostname() + " 401 " + client.getNickname() + " " + ct->second + " :No such nick/channel\r\n" + std::string(EN);
					send(client.getFd(), msg.c_str(), msg.size(), 0); // ERR_NOSUCHNICK
				}
				removeOperator(targetIt->getFd());
				it = modeBools.find(o);
				it->second = false;
				break;
			default:
				msg = std::string(RED) + ":" + server.getHostname() + " 472 " + client.getNickname() + " " + ct->first + " :is unknown mode char to me\r\n" + std::string(EN);
				send(client.getFd(), msg.c_str(), msg.size(), 0); // ERR_ MODE NOT AVAILABLE
				break;
			}
		}
	}
	// Switch statements
}

std::string Channel::generateRPL_CHANNELMODEIS(Client &client, Server &server)
{
	std::string modeString = "+";
	std::string modeArgs;
	// Check which modes are active in the channel
	std::map<std::string, bool>::iterator it = modeBools.find("i");
	if (it->second == true)
		modeString += "i";
	it = modeBools.find("t");
	if (it->second == true)
		modeString += "t";
	it = modeBools.find("k");
	if (it->second == true)
	{
		modeString += "k";
		if (isOperator(client.getFd()))
			modeArgs += " " + getKey();
	}
	it = modeBools.find("o");
	int i = 0;
	for (std::vector<int>::iterator it = operFds.begin(); it != operFds.end(); ++it)
	{
		i++;
		if (i < 2)
			modeString += "o";
		std::stringstream ss;
		std::vector<Client>::iterator ourOperator = server.getClient(*it);
		Client &ourOper = *ourOperator;
		modeArgs += " " + ourOper.getNickname();
		// break;
	}
	it = modeBools.find("l");
	if (it->second == true)
	{
		modeString += "l";
		std::stringstream ss;
		ss << getMax(); // Convert integer to string
		modeArgs += " " + ss.str();
	}
	// Format response
	std::string response = std::string(YEL) + ":" + server.getHostname() + " 324 " + client.getNickname() + " " + this->channelName + " :" + modeString + (modeArgs.empty() ? "" : " " + modeArgs) + "\r\n" + std::string(EN);
	send(client.getFd(), response.c_str(), response.size(), 0);
	return (response);
}
