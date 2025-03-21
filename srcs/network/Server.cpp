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
		if (message.find("/CAP LS") != std::string::npos)
			sendCapabilities(fd);
		else if (message.rfind("/PASS ", 0) == 0)
			validatePassword(fd, message);
		else if (message.rfind("/NICK ", 0) == 0)
			processNickUser(fd, message);
		else if (message.rfind("/USER ", 0) == 0)
			processUser(fd, message);
		else if (message.rfind("/INVITE ", 0) == 0)
			inviteCommand(fd, message);
		else if (message.rfind("/KICK ", 0) == 0)
			kickCommand(fd, message);
		else if (message.rfind("/TOPIC ", 0) == 0)
			topicCommand(fd, message);
		else if (message.find("/CAP REQ") != std::string::npos)
			processCapReq(fd, message);
		else if (message.find("/QUIT", 0) == 0)
			processQuit(fd, message);
		else if (message.find("/JOIN", 0) == 0)
			handleChannel(fd, message); /*Function where JOIN is handled*/
		else if (message.find("/PRIVMSG", 0) == 0)
			processPrivmsg(fd, message);
		else if (message.find("/AUTHENTICATE") != std::string::npos)
			processSasl(fd, message);
		else if (message.find("/CAP END") != std::string::npos)
			capEnd(fd);
		else if (message.find("/MODE") != std::string::npos)
			handleMode(fd, message);
		else
		{
			std::string buff = "Invalid Command: try again!\n";
			send(fd, buff.c_str(), buff.size(), 0);
			// std::cout << YEL << "Client <" << fd << "> Data: " << WHI << buff;
		} //handling authentication error to be displayed to the client.
	}
}

void Server::reverseRotate(std::stack<std::string>& s)
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

	if (!gethostname(this->hostname, sizeof(this->hostname)))
		std::cout << GRE << "Server <" << serSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Listening on " << this->hostname << " on " << this->port << " \r\n";
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
	if (message.rfind("/PASS", 0) == 0)
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
    if (parts.size() < 5 || parts[0] != "/USER")
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
	std::cout << "\033[1;34m===============================================\033[0m" << std::endl;
    std::cout << "\033[1;32m          IRC Command List and Format        \033[0m" << std::endl;
    std::cout << "\033[1;34m===============================================\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/CAP LS  | \033[1;37m /CAP LS\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/PASS    | \033[1;37m /PASS <password>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/NICK    | \033[1;37m /NICK <nickname>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/USER    | \033[1;37m /USER <username> <hostname> <servername> <realname>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/INVITE  | \033[1;37m /INVITE <nickname> <channel>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/KICK    | \033[1;37m /KICK <channel> <nickname> [<reason>]\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/TOPIC   | \033[1;37m /TOPIC <channel> [<topic>]\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/CAP REQ | \033[1;37m /CAP REQ <capability>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/QUIT    | \033[1;37m /QUIT [<message>]\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/JOIN    | \033[1;37m /JOIN <channel> [<key>]\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/PRIVMSG | \033[1;37m /PRIVMSG <target> <message>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/AUTHENTICATE | \033[1;37m /AUTHENTICATE <data>\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/CAP END | \033[1;37m /CAP END\033[0m" << std::endl;

    std::cout << "\033[1;33m* \033[0m";
    std::cout << "/MODE    | \033[1;37m /MODE <target> <mode>\033[0m" << std::endl;

    std::cout << "\033[1;34m===============================================\033[0m" << std::endl;
    std::cout << "\033[1;32m            End of Command List               \033[0m" << std::endl;
    std::cout << "\033[1;34m===============================================\033[0m" << std::endl;
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
	if (message.rfind("/NICK ", 0) == 0)
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
	if (message.find("/AUTHENTICATE PLAIN") != std::string::npos)
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
			std::string error = std::string(RED) + "404 Cannot send to channel " + target + "\r\n" + std::string(WHI);
			send(fd, error.c_str(), error.size(), 0); // ERR_CANNOTSENDTOCHAN
			return;
		}
		// Check if the user is banned or not allowed in the channel
		Channel& channel = it->second;
		if (!channel.isInChannel(fd) || channel.isBanned(sender.getNickname()))
		{
			std::string error = std::string(RED) + "404 Cannot send to channel " + target + "\r\n" + std::string(WHI);
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
			std::string error = std::string(RED) + "401: No such nickname " + target + "\r\n" + std::string(WHI);
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
