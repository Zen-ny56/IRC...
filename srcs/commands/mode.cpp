#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include "../../include/Channel.hpp"

std::map<std::string, std::string> *Server::parseMode(const std::string &message)
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
	// Debug Output
	//
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
	std::cout << "\n\nMODE MAP:\n";
	for (std::map<std::string, std::string>::iterator it = modeMap.begin(); it != modeMap.end(); ++it)
	{
		std::cout << it->first << " -> " << (it->second.empty() ? "(no param)" : it->second) << std::endl;
	}
	std::map<std::string, std::string> *returnedMap = new std::map<std::string, std::string>(modeMap); // Allocate and copy the content of modeMap
	return (returnedMap);
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
		std::string msg = std::string(RED) + ":" + this->hostname + " 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n" + std::string(WHI);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
	}
	Channel &channel = bt->second;
	if (!channel.isOperator(fd))
	{
		std::string msg = std::string(RED) + ":" + this->hostname + " 482 " + client.getNickname() + " " + channel.getChannelName() + " :You're not channel operator\r\n" + std::string(WHI);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
	}
	std::map<std::string, std::string> modes = (*parseMode(message));
	std::map<std::string, bool> modeBool = channel.getModes();
	if (modes.empty())
		generateRPL_CHANNELMODEIS(client, channel, modeBool, fd);
	else
		executeMode(client, channel, modes, modeBool, fd);
}

void Server::executeMode(Client &client, Channel &channel, std::map<std::string, std::string> &modeMap, std::map<std::string, bool> &modeBool, int fd)
{
	for (std::map<std::string, std::string>::iterator ct = modeMap.begin(); ct != modeMap.end(); ++ct)
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
		if (modeStr[0] == '+')
		{
			switch (modeType)
			{
			case 'l':
				// Check if it's valid integer and convert
				if (isNumber(ct->second) == false)
				{
					msg = std::string(RED) + ":ircserv 461 " + client.getNickname() + " " + channel.getChannelName() + " :Not enough parameters" + std::string(WHI);
					send(fd, msg.c_str(), msg.size(), 0);
					return;
				}
				limit = stringToInt(ct->second);
				if (limit <= channel.countInchannel())
				{
					std::ostringstream oss;
					oss << limit;
					std::string errorMsg = std::string(RED) + ":" + this->hostname + " 471 " + client.getNickname() + " " + channel.getChannelName() + " :(+l) input size " + oss.str() + " is less or equal to clients in channel\r\n" + std::string(EN);
					send(fd, errorMsg.c_str(), errorMsg.size(), 0);
					return;
				}
				channel.setMax(limit);
				modeBool[l] = true;
				// msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				// channel.broadcastToChannel(msg);
				break;
			case 'k':
				channel.setKey(ct->second);
				modeBool[k] = true;
				// msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				// channel.broadcastToChannel(msg);
				break;
			case 't':
				channel.setTopRes(true);
				modeBool[t] = true;
				// std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				break;
			case 'i':
				channel.setInviteOnly(true);
				modeBool[i] = true;
				// std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				break;
			case 'o':
				targetIt = getClientUsingNickname(ct->second);
				if (targetIt == clients.end())
				{
					msg = std::string(RED) + ":" + this->hostname + " 401 " + client.getNickname() + " " + ct->second + " :No such nick/channel\r\n" + std::string(EN);
					send(fd, msg.c_str(), msg.size(), 0); // ERR_NOSUCHNICK
				}
				channel.addOperator(targetIt->getFd());
				modeBool[o] = true;
				break;
			default:
				msg = std::string(RED) + ":" + this->hostname + " 472 " + client.getNickname() + " " + ct->first + " :is unknown mode char to me\r\n" + std::string(EN);
				send(fd, msg.c_str(), msg.size(), 0); // ERR_ MODE NOT AVAILABLE
				break;
			}
		}
		else
		{
			switch (modeType)
			{
			case 'l':
				// Check if it's valid integer and convert
				channel.setMax(INT_MAX);
				modeBool[l] = false;
				// msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				// channel.broadcastToChannel(msg);
				break;
			case 'k':
				channel.setKey("");
				modeBool[k] = false;
				// msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				// channel.broadcastToChannel(msg);
				break;
			case 't':
				channel.setTopRes(false);
				modeBool[t] = false;
				// std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				break;
			case 'i':
				channel.setInviteOnly(false);
				modeBool[i] = false;
				// std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
				break;
			case 'o':
				targetIt = getClientUsingNickname(ct->second);
				if (targetIt == clients.end())
				{
					msg = std::string(RED) + ":" + this->hostname + " 401 " + client.getNickname() + " " + ct->second + " :No such nick/channel\r\n" + std::string(EN);
					send(fd, msg.c_str(), msg.size(), 0); // ERR_NOSUCHNICK
				}
				channel.removeOperator(targetIt->getFd());
				modeBool[o] = false;
				break;
			default:
				msg = std::string(RED) + ":" + this->hostname + " 472 " + client.getNickname() + " " + ct->first + " :is unknown mode char to me\r\n" + std::string(EN);
				send(fd, msg.c_str(), msg.size(), 0); // ERR_ MODE NOT AVAILABLE
				break;
			}
		}
	}
	// Switch statements
}

std::string Server::generateRPL_CHANNELMODEIS(Client &client, Channel &channel, std::map<std::string, bool> &modeBool, int fd)
{
	std::string modeString = "+";
	std::string modeArgs;
	std::vector<int> operators = channel.getOperFds();
	std::cout << " RIGHT HERE " << std::endl;
	// Check which modes are active in the channel
	if (modeBool.find("i") != modeBool.end() && modeBool.at("i"))
		modeString += "i";
	if (modeBool.find("t") != modeBool.end() && modeBool.at("t"))
		modeString += "t";
	if (modeBool.find("k") != modeBool.end() && modeBool.at("k") && channel.isOperator(fd))
	{
		modeString += "k";
		if (channel.isOperator(fd))
			modeArgs += " " + channel.getKey();
	}
	if (modeBool.find("o") != modeBool.end())
	{
		for (std::vector<int>::iterator it = operators.begin(); it != operators.end(); ++it)
		{
			modeString += "o";
			break;
		}
	}
	if (modeBool.find("l") != modeBool.end() && modeBool.at("l"))
	{

		modeString += "l";

		std::stringstream ss;
		ss << channel.getMax(); // Convert integer to string
		modeArgs += " " + ss.str();
	}
	// Format response
	std::string response = std::string(YEL) + ":" + this->hostname + +" 324 " + client.getNickname() + " " + channel.getChannelName() + " " + modeString + (modeArgs.empty() ? "" : " " + modeArgs) + "\r\n" + std::string(EN);
	send(fd, response.c_str(), response.size(), 0);
	return (response);
}
