#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include "../../include/Channel.hpp"

std::map<std::string, std::string>*  Server::parseMode(const std::string& message)
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
        		if ((token[i] == '-' && token[i - 1] == '+') || (token[i] == '+' && token[i - 1] == '-'))// Ignore alternating "+-" patterns (e.g., "+-i" or "-+o")
				{
            		continue;  // Skip ineffective mode
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
		std::string bojo = modeWitPams.top(); modeWitPams.pop();
		std::string yoyo = params.top(); params.pop();
		modeMap[bojo] = yoyo;
	}
	if (!params.empty())
	{
		std::string extraf = params.top(); params.pop(); 
		falseParams.push(extraf);
	}
	if (!modeWitPams.empty())
	{
		std::string emptyPam = modeWitPams.top(); modeWitPams.pop();
		modeMap[emptyPam] = "";
	}
	std::cout << "\n\nMODE MAP:\n";
	for (std::map<std::string, std::string>::iterator it = modeMap.begin(); it != modeMap.end(); ++it)
	{
		std::cout << it->first << " -> " << (it->second.empty() ? "(no param)" : it->second) << std::endl;
	}
	std::map<std::string, std::string>* returnedMap = new std::map<std::string, std::string>(modeMap); // Allocate and copy the content of modeMap
	return (returnedMap);
}

void Server::handleMode(int fd, const std::string& message)
{
	std::istringstream iss(message);
	std::string command, channelName;
	std::map<std::string, std::string> modes = (*parseMode(message));
	executeMode(message, modes, fd);

}

void Server::executeMode(const std::string& message, std::map<std::string, std::string>& modeMap, int fd)
{
	//Retreive Client
    std::istringstream iss(message);
	std::string command, channelName;
    iss >> command;
    iss >> channelName;
	std::vector<Client>::iterator it = getClient(fd);
	if (it == clients.end())
		throw std::runtime_error("Client was not found]\n");
	Client& client = (*this)[it];
	//Retrieve Channel
	std::map<std::string, Channel>::iterator bt = channels.find(channelName);
	if (bt == channels.end())
	{
		std::string msg = std::string(RED) + ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel" + std::string(WHI);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
 	}
	Channel& channel = bt->second;
	if (!channel.isOperator(fd))
	{
	 	std::string msg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channel.getChannelName() + " :You're not channel operator\r\n" + std::string(WHI);
		send(fd, msg.c_str(), msg.size(), 0);
		return;
	}
	for (std::map<std::string, std::string>::iterator ct = modeMap.begin(); ct != modeMap.end(); ++ct)
	{
		std::string modeStr = ct->first; // Example: "+l"
		std::string msg;
		int limit;
		char modeType = modeStr[1];
		if (modeStr[0] == '+')
		{
			switch (modeType)
			{
				case 'l':
					// Check if it's valid integer and convert
					if (isNumber(ct->second) == false)
					{
						msg = std::string(RED) + ":ircserv 403 " + client.getNickname() + " " + channel.getChannelName() + " :No such channel" + std::string(WHI);
						send(fd, msg.c_str(), msg.size(), 0);
						return;
					}
					limit = stringToInt(ct->second);
					channel.setMax(limit);
					msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
					channel.broadcastToChannel(msg);
					break;
				case 'k':
					channel.setKey(ct->second);
					msg = "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
					channel.broadcastToChannel(msg);
        			break;
				case 't':
					channel.setTopRes(true);
					std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
					break;
				case 'i':
					channel.setInviteOnly(true);
					std::cout << "MODE on " + channel.getChannelName(); + " " + modeStr + " by " + client.getNickname() + "\r\n";
        			break;
				case 'o':
 					std::vector<Client>::iterator targetIt = getClientUsingNickname(ct->second);
            		if (targetIt == clients.end())
            		{
                		msg = std::string(RED) + ":ircserv 401 " + ct->second + " :No such user\r\n" + std::string(EN);
                		send(fd, msg.c_str(), msg.size(), 0); // ERR_NOSUCHNICK
            		}
					channel.addOperator(fd);
					break; 
    			default:
        			std::cerr << "Unknown mode: " << modeStr << "\n";
       				 break;
}
		}
		
	}
	//Switch statements
}

std::string Server::generateRPL_CHANNELMODEIS(Client& client, Channel& channel, int fd)
{
	std::string modeString = "+";
	std::string modeArgs;
	std::vector<int> operators = channel.getOperFds();
	std::map<std::string, bool> modes = channel.getModes();
	// Check which modes are active in the channel
	if (modes.find("i") != modes.end() && modes.at("i"))
		modeString += "i";
	if (modes.find("t") != modes.end() && modes.at("t"))
		modeString += "t";
	if (modes.find("k") != modes.end() && modes.at("k") && channel.isOperator(fd))
	{
		modeString += "k";
		modeArgs += " " + channel.getKey();
	}
	if (modes.find("o") != modes.end())
	{
		for (std::vector<int>::iterator it = operators.begin(); it != operators.end(); ++it)
		{
			modeString += "o";
			break;
		}
	}
	if (modes.find("l") != modes.end() && modes.at("l"))
	{
		modeString += "l";
		modeArgs += " " + std::to_string(channel.getMax());
	}
	// Format response
	std::string response = std::string(YEL) + ":ircserv 324 " + client.getNickname() + " " + channel.getChannelName() + " " + modeString + (modeArgs.empty() ? "" : " " + modeArgs) + "\r\n" + std::string(WHI);
	return (response);
}
