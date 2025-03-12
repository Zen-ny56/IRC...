#include "../../include/Server.hpp"

Server::Server(){serSocketFd = -1;}

void Server::clearClients(int fd)
{ 	//-> clear the clients
	for(size_t i = 0; i < fds.size(); i++){ //-> remove the client from the pollfd
		if (fds[i].fd == fd)
			{fds.erase(fds.begin() + i); break;}
	}
	for(size_t i = 0; i < clients.size(); i++){ //-> remove the client from the vector of clients
		if (clients[i].getFd() == fd)
			{clients.erase(clients.begin() + i); break;}
	}
}

bool Server::signal = false; //-> initialize the static boolean

void Server::signalHandler(int signum)
{
	(void)signum;
	std::cout << std::endl << "Signal Received!" << std::endl;
	Server::signal = true; //-> set the static boolean to true to stop the server
}

void	Server::closeFds()
{
	for (size_t i = 0; i < clients.size(); i++){ //-> close all the clients
		std::cout << RED << "Client <" << clients[i].getFd() << "> Disconnected" << WHI << std::endl;
		close(clients[i].getFd());
	}
	if (serSocketFd != -1){ //-> close the server socket
		std::cout << RED << "Server <" << serSocketFd << "> Disconnected" << WHI << std::endl;
		close(serSocketFd);
	}
}

void Server::receiveNewData(int fd)
{
	char buff[1024]; //-> buffer for the received data
	memset(buff, 0, sizeof(buff)); //-> clear the buffer

	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1 , 0); //-> receive the data
	if (bytes <= 0)
	{ //-> check if the client disconnected
		std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
		clearClients(fd); //-> clear the client
		close(fd); //-> close the client socket
	} else 
	{ //-> print the received data
		buff[bytes] = '\0';
		std::string message(buff);
		if (message.find("CAP LS") != std::string::npos)
			sendCapabilities(fd);
		else if (message.rfind("PASS ", 0) == 0)
			validatePassword(fd, message);
		else if (message.rfind("NICK ", 0) == 0)
			processNickUser(fd, message);
		else if (message.rfind("USER ", 0) == 0)
			processUser(fd, message);
		else if (message.find("CAP REQ") != std::string::npos)
			processCapReq(fd, message);
		else if (message.find("QUIT", 0) == 0)
			processQuit(fd, message);
		else if (message.find("JOIN", 0) == 0)
			handleChannel(fd, message); /*Function where JOIN is handled*/
		else if (message.find("PRIVMSG", 0) == 0)
			processPrivmsg(fd, message);
		else if (message.find("AUTHENTICATE") != std::string::npos)
			processSasl(fd, message);
		else if (message.find("CAP END") != std::string::npos)
			capEnd(fd);
		else if (message.find("MODE") != std::string::npos)
			handleMode(fd, message);
		else
			std::cout << YEL << "Client <" << fd << "> Data: " << WHI << buff;
	}
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

// void Server::handleMode(int fd, const std::string& message)
// {
// 	std::istringstream iss(message);
// 	std::string command;
// 	std::string channelName;
// 	std::string mode;
// 	std::string param;
	
// 	iss >> command;
// 	std::vector<Client>::iterator iter = getClient(fd);
// 	if (iter == clients.end())
// 		throw std::runtime_error("Error finding client\n");
// 	Client& client = (*this)[iter];
// 	if (!(iss >> channelName))
// 	{ std::cout << RED << "Error: Empty parameter" << WHI << std::endl; return; }
// 	std::map<std::string, Channel>::iterator it = channels.find(channelName);
// 	if (it == channels.end())
// 	{
// 		std::string msg = std::string(RED) + "403 " + client.getNickname() + " " + channelName + " :No such channel" + std::string(WHI);
// 		send(fd, msg.c_str(), msg.size(), 0);
// 		return;
//  	}
// 	Channel& channel = it->second;
// 	// if (!channel.isOperator(fd))
// 	// {
// 	// 	std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 	// 	send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 	// 	return;
// 	// }
// 	if (!(iss >> mode) && !(iss >> param))
// 	{
// 		// If no mode is specified, return the current channel modes
//         std::string response = generateRPL_CHANNELMODEIS(client, channel, fd);
//         send(fd, response.c_str(), response.size(), 0);
//         return;
// 	}
// 	if (!mode.compare("+o") || !mode.compare("-o"))
// 	{
// 		iss >> param;
// 		if (!channel.isOperator(fd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		std::map<std::string, int>::iterator it = nicknameMap.find(param);
// 		if (it == nicknameMap.end())
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 441 " + client.getNickname() + " " + channel.getChannelName() + " :They aren't on the channel\r\n";
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		int targetFd = it->second;
// 		if (!channel.isInChannel(targetFd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 441 " + client.getNickname() + " " + channel.getChannelName() + " :They aren't on the channel\r\n";
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		if (!mode.compare("+o"))
// 		{
// 			channel.addOperator(targetFd);
// 			std::string msg = std::string(GRE) + client.getNickname() + " sets mode +o " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 		else if (!mode.compare("-o"))
// 		{
// 			channel.removeOperator(targetFd);
// 			std::string msg = std::string(RED) + client.getNickname() + " sets mode -o " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 	}
// 	else if (!mode.compare("+k") || !mode.compare("-k"))
// 	{
// 		iss >> param;
// 		if (!channel.isOperator(fd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		if (!mode.compare("+k"))
// 		{
// 			channel.setKey(param);
// 			resetModeBool(channel, mode, true);
// 			std::string msg = std::string(GRE) + client.getNickname() + " sets mode +k " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 		else if (!mode.compare("-k"))
// 		{
// 			channel.setKey("");
// 			resetModeBool(channel, mode, false);
// 			std::string msg = std::string(RED) + client.getNickname() + " sets mode -k " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 	}
// 	else if (!mode.compare("+l") || !mode.compare("-l"))
// 	{
// 		iss >> param;
// 		if (!channel.isOperator(fd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		int limit;
// 		if (isNumber(param))
// 			limit = stringToInt(param);
// 		else
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv  696 " + client.getNickname() + " " + channel.getChannelName() + " l " + param + " :Invalid mode parameter\r\n";
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 		}
// 		if (!mode.compare("+l"))
// 		{
// 			channel.setMax(limit);
// 			resetModeBool(channel, mode, true);
// 			std::string msg = std::string(GRE) + client.getNickname() + " sets mode +l " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 		if (!mode.compare("-l"))
// 		{
// 			channel.setMax(INT_MAX);
// 			resetModeBool(channel, mode, false);
// 			std::string msg = std::string(RED) + client.getNickname() + " sets mode -l " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 	}
// 	else if (!mode.compare("+i") || !mode.compare("-i"))
// 	{
// 		iss >> param;
// 		if (!channel.isOperator(fd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		if (!mode.compare("+i"))
// 		{
// 			channel.setInviteOnly(true);
// 			resetModeBool(channel, mode, true);
// 			std::string msg = std::string(GRE) + client.getNickname() + " sets mode +i " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 		else if (!mode.compare("-i"))
// 		{
// 			channel.setInviteOnly(false);
// 			resetModeBool(channel, mode, false);
// 			std::string msg = std::string(RED) + client.getNickname() + " sets mode -i " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 	}
// 	else if (!mode.compare("+t") || !mode.compare("-t"))
// 	{
// 		iss >> param;
// 		if (!channel.isOperator(fd))
// 		{
// 			std::string errorMsg = std::string(RED) + ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not a channel operator\r\n" + std::string(WHI);
// 			send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			return;
// 		}
// 		if (!mode.compare("+t"))
// 		{
// 			channel.setTopRes(true);
// 			resetModeBool(channel, mode, true);
// 			std::string msg = std::string(GRE) + client.getNickname() + " sets mode +t " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 		else if (!mode.compare("-t"))
// 		{
// 			channel.setTopRes(false);
// 			resetModeBool(channel, mode, false);
// 			std::string msg = std::string(RED) + client.getNickname() + " sets mode -t " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 			channel.broadcastToChannel(msg);
// 		}
// 	}

// }

void Server::handleMode(int fd, const std::string& message)
{
	(void)fd;
	std::istringstream iss(message);
	std::string command, channelName, token;
	std::map<std::string, std::string> modeMap; // Store mode-parameter pairs
	std::vector<std::string> modeOrder; // Keep track of order of modes needing parameters

	// Read command
	iss >> command;

	// Read channel name
	if (!(iss >> channelName)) {
		std::cerr << "Error: No channel name provided.\n";
		return;
	}

	std::queue<std::string> pendingParams;
	bool expectingParam = false;
	std::string lastMode;

	// Process input
	while (iss >> token) {
		if (!token.empty() && (token[0] == '+' || token[0] == '-')) {
			// New mode group detected, process each mode separately
			expectingParam = false; // Reset because a new mode group starts
			char modeType = token[0];

			for (size_t i = 1; i < token.size(); ++i) {
				std::string mode = std::string(1, modeType) + token[i];

				// If mode is 'i' or 't', it never takes a parameter
				if (token[i] == 'i' || token[i] == 't') {
					modeMap[mode] = ""; // Ensure empty parameter
				} else {
					modeMap[mode] = ""; // Initialize mode
					modeOrder.push_back(mode); // Store the order
					lastMode = mode;
					expectingParam = true; // Expect parameter for this mode
				}
			}
		} else {
			if (expectingParam) {
				// Assign parameter to last mode if expected
				modeMap[lastMode] = token;
				expectingParam = false;
			} else {
				// Store unexpected token
				pendingParams.push(token);
			}
		}
	}

	// **Assign Remaining Parameters in Order**
	for (std::vector<std::string>::iterator it = modeOrder.begin(); it != modeOrder.end(); ++it) {
		if (modeMap[*it].empty() && !pendingParams.empty()) {
			modeMap[*it] = pendingParams.front();
			pendingParams.pop();
		}
	}

	// **Flag any remaining unexpected tokens**
	while (!pendingParams.empty()) {
		std::cerr << "Warning: Unexpected token '" << pendingParams.front() << "'\n";
		pendingParams.pop();
	}

	// **Final Check:** Ensure 'i' and 't' have no parameters
	for (std::map<std::string, std::string>::iterator it = modeMap.begin(); it != modeMap.end(); ++it) {
		if (it->first == "+i" || it->first == "+t") {
			it->second = ""; // Forcefully clear any assigned parameter
		}
	}
		// Debug Output
		std::cout << "MODE MAP:\n";
		for (std::map<std::string, std::string>::iterator it = modeMap.begin(); it != modeMap.end(); ++it)
		{
			std::cout << it->first << " -> " << (it->second.empty() ? "(no param)" : it->second) << std::endl;
		}
// Execute mode with the new format
// executeMode(channelName, modeMap, fd);

	// Execute mode with the new format
	// executeMode(channelName, modeMap, fd);
	// 		switch (modeString[i])
	// 		{
	// 			case 'o': {
    //                 if (paramIndex >= params.size()) break;
    //                 std::string target = params[paramIndex++];
    //                 int targetFd = getClientFdByNickname(target);
    //                 if (targetFd == -1 || !channel.isInChannel(targetFd)) {
    //                     sendError(fd, ":ircserv 441 " + getClient(fd)->getNickname() + " " + channelName + " :They aren't on the channel");
    //                     break;
    //                 }
    //                 if (sign == '+') channel.addOperator(targetFd);
    //                 else channel.removeOperator(targetFd);
    //                 break;
    //             }
    //             case 'k': {
    //                 if (paramIndex >= params.size()) break;
    //                 if (!channel.isOperator(fd)) {
    //                     sendError(fd, ":ircserv 482 " + getClient(fd)->getNickname() + " " + channelName + " :You're not a channel operator");
    //                     break;
    //                 }
    //                 channel.setKey(sign == '+' ? params[paramIndex++] : "");
    //                 break;
    //             }
    //             case 'l': {
    //                 if (sign == '+' && paramIndex < params.size()) {
    //                     channel.setMax(std::stoi(params[paramIndex++]));
    //                 } else {
    //                     channel.setMax(INT_MAX);
    //                 }
    //                 break;
    //             }
    //             case 'i':
    //                 channel.setInviteOnly(sign == '+');
    //                 break;
    //             case 't':
    //                 channel.setTopRes(sign == '+');
    //                 break;
    //             default:
    //                 sendError(fd, ":ircserv 472 " + getClient(fd)->getNickname() + " " + modeString[i] + " :Unknown mode");
    //                 break;
    //         }
    //     }
    // }
}

// void Server::executeMode(std::string channelName, std::vector<std::string>& modeTokens, std::vector<std::string>& params, int fd)
// {
// 	//Retreive Client
// 	std::vector<Client>::iterator it = getClient(fd);
// 	if (it == clients.end())
// 		throw std::runtime_error("Client was not found]\n");
// 	Client& client = (*this)[it];
// 	//Retrieve Channel
// 	std::map<std::string, Channel>::iterator bt = channels.find(channelName);
// 	if (bt == channels.end())
// 	{
// 		std::string msg = std::string(RED) + "403 " + client.getNickname() + " " + channelName + " :No such channel" + std::string(WHI);
// 		send(fd, msg.c_str(), msg.size(), 0);
// 		return;
//  	}
// 	Channel& channel = bt->second;

// 	//Switch statements
// 	for (std::vector<std::string>::iterator ct = modeTokens.begin(); ct != modeTokens.end(); ++ct)
// 	{
// 		std::string temp = *ct;
// 		char sign = temp[0];
// 		char letter = temp[1];
// 		if (sign == '-')
// 		{
// 			switch (letter)
// 			{
// 				case 'k':
// 				{
// 					channel.setKey("");
// 					resetModeBool(channel, temp, false);
// 					std::string msg = std::string(RED) + client.getNickname() + " sets mode -k " + " on " + channel.getChannelName() + "\r\n" + std::string(WHI);
// 					channel.broadcastToChannel(msg);
// 				}
// 				case 'o':
// 				{
// 					std::map<std::string, int>::iterator dt = nicknameMap.find(param);
// 					if (dt == nicknameMap.end())
// 					{
// 						std::string errorMsg = std::string(RED) + ":ircserv 441 " + client.getNickname() + " " + channel.getChannelName() + " :They aren't on the channel\r\n";
// 						send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 						return;
// 					}
// 					int targetFd = dt->second;
// 					if (!channel.isInChannel(targetFd))
// 					{
// 						std::string errorMsg = std::string(RED) + ":ircserv 441 " + client.getNickname() + " " + channel.getChannelName() + " :They aren't on the channel\r\n";
// 						send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 						return;
// 					}
// 				}
// 			}
// 		}
// 	}
// 		std::string parameter;
// 		std::vector<std::string>::iterator dt;
// 		if (!temp.compare("+l") || !temp.compare("-l") || !temp.compare("+k") || !temp.compare("-k"))
// 		{
// 			for (dt = params.begin(); dt != params.end(); ++dt)
// 			{
// 				parameter = *dt;
// 			}
// 			if (dt == params.end())
// 			{
// 				std::string errorMsg = std::string(RED) + ":ircserv  696 " + client.getNickname() + " " + channel.getChannelName() + " " + ct[1] + " " + *dt + " :Invalid mode parameter\r\n";
// 				send(fd, errorMsg.c_str(), errorMsg.size(), 0);
// 			}
			
// 		}
// 	}
// }

void Server::resetModeBool(Channel &channel, std::string mode, bool condition)
{
	std::string extracted;
	for (std::string::size_type i = 0; i < mode.length(); i++)
	{
		if (std::isalpha(mode[i]))
			extracted += mode[i];
	}
	std::map<std::string, bool>::iterator it = channel.getModes().find(extracted);
	it->second = condition;
}

void Server::processQuit(int fd, const std::string& reason) 
{
    std::string nickname = clients[fd].getNickname();
	// Compose the QUIT message
	std::string quitMessage = ":" + nickname + " QUIT :Quit: " + (reason.empty() ? "" : reason);
	// Notify all clients sharing channels with the quitting client
	// broadcastToSharedChannels(fd, quitMessage); // Assume this function broadcasts to all relevant clients
    // std::string errorMessage = "ERROR :Closing link (" + nickname + ") [Quit: " + reason + "]";
	// send(fd, quitMessage.c_str(), quitMessage.size(), 0);
    // Remove the client from the server
    disconnectClient(fd); // Assume this function handles removing the client from all data structures and closing the connection
}

void Server::disconnectClient(int fd)
{
	clearClients(fd);
	close(fd);
	std::cout << RED << "Client <" << fd <<  "> Disconnected" << std::endl;
}

void Server::acceptNewClient()
{
	Client cli; //-> create a new client
	struct sockaddr_in cliadd;
	struct pollfd newPoll;
	socklen_t len = sizeof(cliadd);

	int incofd = accept(serSocketFd, (sockaddr *)&(cliadd), &len); //-> accept the new client
	if (incofd == -1)
		{std::cout << "accept() failed" << std::endl; return;}
	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		{std::cout << "fcntl() failed" << std::endl; return;}

	newPoll.fd = incofd; //-> add the client socket to the pollfd
	newPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	newPoll.revents = 0; //-> set the revents to 0

	cli.setFd(incofd); //-> set the client file descriptor
	cli.setIpAdd(inet_ntoa((cliadd.sin_addr))); //-> convert the ip address to string and set it
	clients.push_back(cli); //-> add the client to the vector of clients
	fds.push_back(newPoll); //-> add the client socket to the pollfd

	// authenticatedClients[incofd] = false;
	std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::serSocket()
{
	int en = 1;
	struct sockaddr_in add;
	struct pollfd newPoll;
	add.sin_family = AF_INET; //-> set the address family to ipv4
	add.sin_addr.s_addr = INADDR_ANY; //-> set the address to any local machine address
	add.sin_port = htons(this->port); //-> convert the port to network byte order (big endian)

	serSocketFd = socket(AF_INET, SOCK_STREAM, 0); //-> create the server socket
	if(serSocketFd == -1) //-> check if the socket is created
		throw(std::runtime_error("failed to create socket"));
	if(setsockopt(serSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) //-> set the socket option (SO_REUSEADDR) to reuse the address
		throw(std::runtime_error("failed to set option (SO_REUSEADDR) on socket"));
	 if (fcntl(serSocketFd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		throw(std::runtime_error("failed to set option (O_NONBLOCK) on socket"));
	if (bind(serSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) //-> bind the socket to the address
		throw(std::runtime_error("failed to bind socket"));
	if (listen(serSocketFd, SOMAXCONN) == -1) //-> listen for incoming connections and making the socket a passive socket
		throw(std::runtime_error("listen() failed"));
	newPoll.fd = serSocketFd; //-> add the server socket to the pollfd
	newPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	newPoll.revents = 0; //-> set the revents to 0
	fds.push_back(newPoll); //-> add the server socket to the pollfd
}

void Server::serverInit(int port, std::string pass)
{
	this->port = port;
	this->password = pass;
	serSocket(); //-> create the server socket

	std::cout << GRE << "Server <" << serSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Waiting to accept a connection...\n";

	while (Server::signal == false)
	{ //-> run the server until the signal is received

		if ((poll(&fds[0],fds.size(),-1) == -1) && Server::signal == false) //-> wait for an event
			throw(std::runtime_error("poll() failed"));

		for (size_t i = 0; i < fds.size(); i++){ //-> check all file descriptors
			if (fds[i].revents & POLLIN)
			{ //-> check if there is data to read
				if (fds[i].fd == serSocketFd)
					acceptNewClient(); //-> accept new client
				else
					receiveNewData(fds[i].fd); //-> receive new data from a registered client
			}
		}
	}
	closeFds(); //-> close the file descriptors when the server stops
}

void Server::sendCapabilities(int fd)
{
	std::string capMessage = "CAP * LS :multi-prefix sasl\r\n";
	send(fd, capMessage.c_str(), capMessage.size(), 0);
	return ;
}

void Server::processCapReq(int fd, const std::string& message)
{
	if (message.find("CAP REQ") != std::string::npos){
		std::string capAck = "CAP * ACK :multi-prefix sasl\r\n";
		send(fd, capAck.c_str(), capAck.size(), 0);
		return ;
	}
}

void Server::validatePassword(int fd, const std::string& message)
{
	if (message.rfind("PASS", 0) == 0)
	{ // Check if message starts with "PASS"
		std::vector<Client>::iterator it = getClient(fd);
		if (it == clients.end())
			throw std::runtime_error("No client was found\n");
		Client& client = (*this)[it];
		std::string receivedPassword = message.substr(5); // Extract password
		receivedPassword.erase(0, receivedPassword.find_first_not_of(" \t\r\n")); // Remove leading whitespace
		receivedPassword.erase(receivedPassword.find_last_not_of(" \t\r\n") + 1); // Remove trailing whitespace
		if (client.getPassAuthen() == true)
		{
			std::string errMsg = std::string(RED) + "462 PASS: You may not register\r\n" + std::string(WHI);
			send(fd, errMsg.c_str(), errMsg.size(), 0);
			return ;
		}
		if (receivedPassword.empty())
		{
			std::string errMsg = std::string(RED) + "461 PASS :Not enough parameters\r\n" + std::string(WHI);
			send(fd, errMsg.c_str(), errMsg.size(), 0); // ERR_NEEDMOREPARAMS
			return ;
		}
		if (receivedPassword.compare(this->password) == 0)
		{
			client.setPassAuthen();
			return ; // Authentication successful
		} 
		else
        {
			std::string errMsg = std::string(RED) + "464 :Password incorrect\r\n" + std::string(WHI);
			send(fd, errMsg.c_str(), errMsg.size(), 0); // ERR_PASSWDMISMATCH
			return ;
        }
	}
	return ; // Authentication failed
}

void Server::processUser(int fd, const std::string& message)
{
	// Split the message into parts
	std::vector<Client>::iterator it = getClient(fd);
	if (it == clients.end())
		throw std::runtime_error("Client was not found]\n");
	Client& client = (*this)[it];
	if (client.getNickAuthen() == false || client.getPassAuthen() == false)
	{
		std::cout << RED << "Entering Here" << WHI << std::endl;
		return ;
	}
	std::istringstream iss(message);
	std::vector<std::string> parts;
	std::string part;
	while (std::getline(iss, part, ' '))
        parts.push_back(part);
    // Check minimum parameter count
    if (parts.size() < 5 || parts[0] != "USER")
	{
		std::string errMsg = std::string(RED) +  "461 USER :Not enough parameters\r\n" + std::string(WHI);
		send(fd, errMsg.c_str(), errMsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	std::string username = parts[1];
	std::string unused1 = parts[2]; // This is usually "0"
	std::string unused2 = parts[3]; // This is usually "*"
	std::string realname = message.substr(message.find(':') + 1);

	// Check if the user is already registered
	if (client.getUserAuthen() == true)
	{
		std::string errMsg = std::string(RED) + "462 :You may not register\r\n" + std::string(WHI);
		send(fd, errMsg.c_str(), errMsg.size(), 0); // ERR_ALREADYREGISTERED
		return;
	}
	if (username.empty() || realname.empty())
	{
		std::string errMsg = std::string(RED) +  "461 USER :Not enough parameters\r\n" + std::string(WHI);
		send(fd, errMsg.c_str(), errMsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	// Register the user
	client.setUserName(username, realname);
	// Log successful processing
	this->sendWelcome(fd, client);
}

void Server::sendWelcome(int fd, Client& client)
{
	// 1. RPL_WELCOME (001)
	std::string welcomeMsg = std::string(YEL) + ":" + "ircserv" + " 001 " + client.getNickname() + " :Welcome to the IRC Network " + client.getNickname() + "!" + client.getUserName() + "@" + client.getIPadd() + "\r\n";
	send(fd, welcomeMsg.c_str(), welcomeMsg.size(), 0);

	// 2. RPL_YOURHOST (002)
	std::string yourHostMsg = std::string(YEL) + ":" + "ircserv" + " 002 " + client.getNickname() + " :Your host is " + "ircserv" + ", running version 1.0" + "\r\n";
	send(fd, yourHostMsg.c_str(), yourHostMsg.size(), 0);

	// 3. RPL_CREATED (003)
	std::string createdMsg = std::string(YEL) + ":" + "ircserv" + " 003 " + client.getNickname() + " :This server was created on 01 Jan 2020" + "\r\n";
	send(fd, createdMsg.c_str(), createdMsg.size(), 0);

	// 4. RPL_MYINFO (004)
	std::string myInfoMsg = std::string(YEL) + ":" + "ircserv" + " 004 " + client.getNickname() + " " + "IRCserv" + " v1.0 :Welcome to IRC Network" + "\r\n" + std::string(WHI);
	send(fd, myInfoMsg.c_str(), myInfoMsg.size(), 0);
}

void Server::processNickUser(int fd, const std::string& message)
{
	// NICK command
	if (message.rfind("NICK ", 0) == 0)
	{
		std::vector<Client>::iterator it = getClient(fd);
		if (it == clients.end())
			throw  std::runtime_error("Client was not found\n");
		Client& client = (*this)[it];
		if (client.getPassAuthen() == false || client.getNickAuthen() == true)
			return;
		std::string nickname = message.substr(5); // Extract nickname
		nickname.erase(0, nickname.find_first_not_of(" \t\r\n"));
		nickname.erase(nickname.find_last_not_of(" \t\r\n") + 1);
		if (nickname.empty())
		{
			std::string errorMsg = std::string(RED) + "431 :No nickname given" + "\r\n" + std::string(WHI);
            send(fd, errorMsg.c_str(), errorMsg.size(), 0); // ERR_NONICKNAMEGIVEN
			return;
		}
		if (!isValidNickname(nickname))
		{
            std::string errorMsg = std::string(RED) + "432 " + nickname + " :Erroneous nickname" + "\r\n" + std::string(WHI); // ERR_ERRONEUSNICKNAME
			send(fd, errorMsg.c_str(), errorMsg.length(), 0);
			return;
		}
		if (nicknameMap.find(nickname) != nicknameMap.end())
		{
 			std::string errorMsg = std::string(RED) + "433 " + nickname + " :Nickname is already in use\r\n" + std::string(WHI); // ERR_NICKNAMEINUSE
			send(fd, errorMsg.c_str(), errorMsg.length(), 0);
			return;
		}
		// Update client's nickname
		// Client& client = getClient(fd);
		std::string oldNickname = client.getNickname();
		if (!oldNickname.empty())
			nicknameMap.erase(oldNickname); // Remove old nickname from the map
		client.setNickname(nickname);
		nicknameMap[nickname] = fd; // Add the new nickname to the map
		std::string response = std::string(GRE) + ":" + oldNickname + " NICK " + client.getNickname() +  "\r\n" + std::string(WHI); // Inform the client of the nickname change
		send(fd, response.c_str(), response.length(), 0);
		std::cout << "Client <" << fd << "> changed nickname to: " << nickname << std::endl;
	}
}

void Server::processSasl(int fd, const std::string& message)
{
	if (message.find("AUTHENTICATE PLAIN") != std::string::npos)
	{
		std::string response = "AUTHENTICATE +\r\n";
		send(fd, response.c_str(), response.size(), 0);
    } else if (message.find("AUTHENTICATE ") == 0) {
		// Decode and validate credentials
		std::string credentials = message.substr(13); // Base64-encoded
		// Decode and verify (requires base64 decoding)
		// Example: Validate "username\0username\0password"
		send(fd, "900 :Authentication successful\r\n", 33, 0);
	}
}

void Server::capEnd(int fd)
{
	std::string capEnd = "CAP END\r\n";
	send(fd, capEnd.c_str(), capEnd.size(), 0);
	return;
}

std::vector<Client>::iterator Server::getClient(int fd)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->getFd() == fd)
			return it;
	}
	return(clients.end());
}

bool Server::isValidNickname(const std::string& nickname)
{
	// Ensure the nickname is not too long or too short
	if (nickname.length() < 1 || nickname.length() > 30) // Adjust max length as per your protocol
		return false;
    // Ensure the first character is not a digit, space, colon, or special character
	char firstChar = nickname[0];
	if (!std::isalpha(firstChar) && firstChar != '[' && firstChar != '{' &&
		firstChar != ']' && firstChar != '}' && firstChar != '\\' &&
		firstChar != '|')
		return false;
	// Ensure no invalid characters are in the nickname
	for (std::string::const_iterator it = nickname.begin(); it != nickname.end(); ++it)
	{
		char c = *it;
		if (!(std::isalnum(c) || c == '[' || c == ']' || c == '{' || c == '}' ||
			c == '\\' || c == '|'))
			return false;
	}
	return true;
}

void Server::handleChannel(int fd, const std::string& message)
{
	//Extract parameters after JOIN , client is going to send JOIN #channel1,#channel2 key1,key2 or JOIN #channel1
	size_t paramsStart = message.find(' ') + 1;
    if (paramsStart == std::string::npos || paramsStart >= message.length())
	{
		std::string errormsg = std::string(RED) + "461 JOIN :Not enough parameters\r\n";
		send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	std::string params = message.substr(paramsStart);
	// Split channels and keys
	size_t spacePos = params.find(' ');
	std::string channelsPart = params.substr(0, spacePos); // Comma-separated channel names
	std::string keysPart = (spacePos != std::string::npos) ? params.substr(spacePos + 1) : ""; // Comma-separated keys
	// Parse channels and keys
	std::vector<std::string> channels = splitByDelimiter(channelsPart, ',');
	std::vector<std::string> keys = splitByDelimiter(keysPart, ',');

	//Iterate through each channel and call joinChannel
	for (size_t i = 0; i < channels.size(); ++i)
	{
		const std::string& channelName = channels[i];
		const std::string& key = (i < keys.size()) ? keys[i] : ""; // Match keys to channels if possible
        if (!isValidChannelName(channelName))
		{
			std::string errormsg = std::string(RED) + "476 " + channelName + " :Invalid channel name\r\n";
			send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_BADCHANMASK
			continue;
		}
		joinChannel(fd, channelName, key);
    }
}

/*Initial parsing is done and we're joining a channel*/
void Server::joinChannel(int fd, const std::string& channelName, const std::string& key)
{
	std::vector<Client>::iterator iter = getClient(fd);
	if (iter == clients.end())
		throw std::runtime_error("Error finding client\n");
	Client& client = (*this)[iter];
	if (client.getUserAuthen() == false)
		return ;
	std::map<std::string, Channel>::iterator it = channels.find(channelName);
	if (it == channels.end())
	{
		// Channel doesn't exist, so create it
		channels[channelName] = Channel(channelName, key, client.getFd());
		it = channels.find(channelName); // Re-get the iterator after creation
	}
	Channel& channel = it->second;
	if (channel.isInChannel(fd))
		return ;
    // 3. Validate conditions for joining the channel
	if (channel.isInviteOnly() && !channel.isInvited(fd))
	{
		std::string errorMsg = std::string(RED) + "473 " + client.getNickname() + " " + channelName + " :Invite-only channel\r\n" + std::string(WHI);
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
    }
    if (channel.isFull())
	{
		std::string errorMsg = std::string(RED) + client.getNickname() + " " + channelName + " :Channel is full\r\n" + std::string(WHI);
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
	}
	if (!channel.getKey().empty() && channel.getKey() != key)
	{
		std::string errorMsg = std::string(RED) + "475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n" + std::string(WHI);
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
    }
	if (channel.isBanned(client.getNickname()))
	{
		std::string errorMsg = std::string(RED) + "474" + client.getNickname() + " :You are banned from this channel\r\n" + std::string(WHI);
        send(fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

	// 4. Add the client to the channel
	channel.addClient(fd);
	std::cout << channel.getKey() << std::endl;
	// 5. Broadcast JOIN message to all clients in the channel
	std::string joinMessage = ":" + client.getNickname() + " JOIN :" + channelName + "\r\n" + std::string(WHI);
	channel.broadcastToChannel(joinMessage);

    // 6. Send the channel topic (or indicate no topic set)
    if (!channel.getTopic().empty())
	{
		std::string info = std::string(YEL) + "332 " + client.getNickname() + " " + channelName + " :" + channel.getTopic() + "\r\n" + std::string(WHI);
		send(fd, info.c_str() ,info.size(), 0);
	} else {
		std::string info = std::string(YEL) + "331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n" + std::string(WHI);
		send(fd, info.c_str(), info.size(), 0);
    }

    // 7. Send the list of users in the channel
	std::vector<int> clientList = channel.listUsers();
	for (std::vector<int>::iterator it = clientList.begin(); it != clientList.end(); ++it)
	{
		std::vector<Client>::iterator bt = getClient(*it);
		if (bt == clients.end())
			throw std::runtime_error("Error finding clients\n");
		Client& user = (*this)[bt];
		std::string msg = std::string(YEL) + "353 " + client.getNickname() + " = " + channelName + " :" +  user.getNickname() + "\r\n" + std::string(WHI);
		send(fd, msg.c_str(), msg.size(), 0); 
	}
	std::string msg = std::string(YEL) + "366 " + client.getNickname() + " " + channelName + " :End of /Names list\r\n" + std::string(WHI);
    send(fd, msg.c_str(), msg.size(), 0);
}

std::vector<std::string> Server::splitByDelimiter(const std::string& str, char delimiter)
{
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delimiter))
	{
		item = trim(item);
		if (!item.empty())
			result.push_back(item);
	}
	return result;
}

bool Server::isValidChannelName(const std::string& channelName)
{
	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
        return false;
	// Additional validation rules can be added here
	return true;
}

Client& Server::operator[](std::vector<Client>::iterator it)
{
	if (it == clients.end())
		throw std::out_of_range("Iterator out of range for clients vector");
	return *it;
}


void Server::processPrivmsg(int fd, const std::string& message)
{
	std::vector<Client>::iterator bt = getClient(fd);
	if (bt == clients.end())
		throw std::runtime_error("Error finding client\n");
	Client& sender = (*this)[bt];
    size_t commandEnd = message.find(' ');
    if (commandEnd == std::string::npos || message.substr(0, commandEnd) != "PRIVMSG") {
        std::string error = std::string(RED) + "421: Unknown command\r\n" + std::string(WHI);
    }
    // Skip the "PRIVMSG" part
    size_t targetStart = commandEnd + 1; // Position after "PRIVMSG "
    size_t spacePos = message.find(' ', targetStart); // Find space after target
    if (spacePos == std::string::npos)
	{
        // If there's no space after the target, no message text is provided
        std::string error = std::string(RED) + "411: No recipient given (PRIVMSG)\r\n" + std::string(WHI);
        send(fd, error.c_str(), error.size(), 0); // ERR_NORECIPIENT
        return;
    }
    // Extract the target
    std::string target = message.substr(targetStart, spacePos - targetStart);
    // Skip any spaces after the target and check for message text
    size_t textStart = message.find_first_not_of(' ', spacePos + 1);
    if (textStart == std::string::npos) {
        // If no text is found after the target, return an error
        std::string error = std::string(RED) + "412: No text to send\r\n" + std::string(WHI);
        send(fd, error.c_str(), error.size(), 0); // ERR_NOTEXTTOSEND
        return;
    }
    // Extract the actual message text
    std::string text = message.substr(textStart);
	if (target[0] == '#')
	{
		std::map<std::string, Channel>::iterator it = channels.find(target);
		if (it == channels.end())
		{
			std::string error = std::string(RED) + "404 Cannot send to channel " + target + "\r\n" + std::string(WHI) + EN; //One of the WHI colors isnt being reset
			send(fd, error.c_str(), error.size(), 0); // ERR_CANNOTSENDTOCHAN
			return;
		}
		// Check if the user is banned or not allowed in the channel
		Channel& channel = it->second;
		if (!channel.isInChannel(fd) || channel.isBanned(sender.getNickname()))
		{
			std::string error = std::string(RED) + "404 Cannot send to channel " + target + "\r\n" + std::string(WHI) + EN; //checking if adding EN will remove white color
			send(fd, error.c_str(), error.size(), 0); // ERR_CANNOTSENDTOCHAN
			return;
		}
		// Send the message to the channel members
		channel.broadcastToChannel(text);
		}
		// std::map<std::string, Channel>::iterator it = channels.find(target);
	/*}*/ else
	{
		std::vector<Client>::iterator ct = getClientUsingNickname(target);
		if (ct == clients.end())
		{
			std::string error = std::string(RED) + "401: No such nickname " + target + "\r\n" + std::string(WHI) + EN;// added EN to remove white color after error message appears
            send(fd, error.c_str(), error.size(), 0); // ERR_NOSUCHNICK
			return ;
		}
		Client& recepient = (*this)[ct];
        // Send the private message to the user
        std::string response = ":" + sender.getNickname() + " PRIVMSG " + recepient.getNickname() + " :" + text + "\r\n";
        send(recepient.getFd(), response.c_str(), response.size(), 0);
    } 
}

// // Helper methods for getting client and checking channels
std::vector<Client>::iterator Server::getClientUsingNickname(const std::string& nickname)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		std::string clientsNick = it->getNickname();
		if (clientsNick.compare(nickname) == 0)
			return it;
	}
	return (clients.end());
}


std::string Server::trim(const std::string& str)
{
    size_t start = str.find_first_not_of("\r\n\t");
    size_t end = str.find_last_not_of("\r\n\t");
    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

bool isNumber(const std::string &str)
{
    for (size_t i = 0; i < str.length(); i++)
	{
        if (!std::isdigit(str[i])) // Check if all characters are digits
            return false;
    }
    return !str.empty(); // Ensure the string isn't empty
}

int stringToInt(const std::string &str)
{
	std::stringstream ss(str);
	int number;
	ss >> number; // Convert string to integer
	return number;
}
