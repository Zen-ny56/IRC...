#include "../../include/Server.hpp"

Server::Server() { serSocketFd = -1; }

void Server::clearClients(int fd)
{
	(void)fd;
	// // Remove from clients vector and all channels
	for (size_t i = 0; i < clients.size(); i++)
	{
		if (clients[i].getFd() == fd)
		{
			// Remove client from all channels
			std::string nickname = clients[i].getNickname();
			for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
			{
				Channel &channel = it->second;
				if (channel.isInChannel(fd))
				{
					// std::cout << "PYSSSSSSSVVFCCCCC" << std::endl;
					if (channel.isInviteOnly() && channel.isInvitedUser(fd))
					channel.removeClientFromInvitation(fd);
					if (!channel.isInvited(fd))
					channel.remove_isInvited(fd);
					// Broadcast quit message to channel members
					std::string quitMsg = ":" + nickname + " QUIT :Client exited\r\n";
					channel.broadcastToChannel(quitMsg);
					channel.removeClient(fd);
					// Remove channel if empty
					if (channel.listUsers().empty())
					{
						channel.removeModes();
						// std::cout << "PYSSSSSSSVVFCCCCC" << std::endl;
						channels.erase(it->first);
					}
				}
			}
			// Remove nickname from map
			nicknameMap.erase(nickname);
			clients.erase(clients.begin() + i);
			break;
		}
	}
	for (size_t i = 0; i < fds.size(); i++)
	{
		if (fds[i].fd == fd)
		{
			close(fds[i].fd);
			fds.erase(fds.begin() + i);
			break;
		}
	}
}

bool Server::signal = false; //-> initialize the static boolean

void Server::signalHandler(int signum)
{
	(void)signum;
	std::cout << std::endl
			  << "Signal Received!" << std::endl;
	Server::signal = true; //-> set the static boolean to true to stop the server
}

void Server::closeFds()
{
	for (size_t i = 0; i < clients.size(); i++)
	{ //-> close all the clients
		std::cout << RED << "Client <" << clients[i].getFd() << "> Disconnected" << WHI << std::endl;
		close(clients[i].getFd());
	}
	if (serSocketFd != -1)
	{ //-> close the server socket
		std::cout << RED << "Server <" << serSocketFd << "> Disconnected" << WHI << std::endl;
		close(serSocketFd);
	}
}

int Server::authenticate(Client& client, const std::string &line, bool needsCap)
{
	(void)needsCap;
    std::istringstream stream(line);
    std::string command, type;
    
	stream >> command;
	// std::cout << command << std::endl;
    if (command == "NICK")
	{
        std::string nickname;
        stream >> nickname;
        std::cout << "Nickname: " << nickname << std::endl;
        processNickUser(client, nickname);
    } 
    else if (command == "USER")
	{
        std::string user, ident, host, realname;
        stream >> user >> ident >> host;
        // Extract the real name (everything after ':')
        std::getline(stream, realname); 
        size_t pos = realname.find(':'); 
        if (pos != std::string::npos)
            realname = realname.substr(pos + 1);
        processUser(client, user, ident, host, realname);
    }
	else if (command == "PASS")
	{
        std::string password;
        stream >> password;
        validatePassword(client, password); // Call existing function
    } 
    else if (command == "CAP")
	{
		std::string capCommand;
		stream >> capCommand;
        if (capCommand == "LS")
            sendCapabilities(client.getFd()); // Call existing function
        else if (capCommand == "REQ")
		{
			std::string requestedCaps;
			std::getline(stream, requestedCaps);
			processCapReq(client.getFd(), requestedCaps); // Call existing function
		}
		else if (capCommand == "END")
		{
			if (!client.ifAuthenticated())
			{
				client.setCapisSuccess(true);
				sendWelcome(client.getFd(), client);
			}
		}
	}
	return (0);
}

void Server::authenticate(Client& client, const std::string &line)
{
    std::istringstream stream(line);
    std::string command, type;
    
	stream >> command;
	std::cout << "Hoped into the wrong place" << std::endl;
	// std::cout << command << std::endl;
    if (command == "NICK")
	{
        std::string nickname;
        stream >> nickname;
        std::cout << "Nickname: " << nickname << std::endl;
        processNickUser(client, nickname);
		if (!client.ifAuthenticated())
			sendWelcome(client.getFd(), client);
    } 
    else if (command == "USER")
	{
        std::string user, ident, host, realname;
        stream >> user >> ident >> host;
        // Extract the real name (everything after ':')
        std::getline(stream, realname); 
        size_t pos = realname.find(':'); 
        if (pos != std::string::npos)
            realname = realname.substr(pos + 1);
        processUser(client, user, ident, host, realname);
		if (!client.ifAuthenticated())
			sendWelcome(client.getFd(), client);
    }
	else if (command == "PASS")
	{
        std::string password;
        stream >> password;
        validatePassword(client, password); // Call existing function
		if (!client.ifAuthenticated())
			sendWelcome(client.getFd(), client);
    } 
}

std::vector<std::string> Server::storeInputLines(Client& client, const std::string &message)
{
    std::vector<std::string> lines;
    std::istringstream inputStream(message);
    std::string line;

    while (std::getline(inputStream, line))
	{
		std::cout << line << std::endl;
        lines.push_back(line);
	}
	if (lines[0].find("CAP LS") == 0)
	{
		client.setNeedsCap(true);
	}
    return lines;
}

void Server::receiveNewData(int fd)
{
	char buff[1024];			   //-> buffer for the received data
	memset(buff, 0, sizeof(buff)); //-> clear the buffer

	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0); //-> receive the data
	// sendPingToClients();
	std::vector<Client>::iterator it = getClient(fd);
	if (it == clients.end())
		throw std::runtime_error("Client was not found]\n");
	Client &client = (*this)[it];
	if (bytes <= 0)
	{ //-> check if the client disconnected
		std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
		clearClients(fd); //-> clear the client
		close(fd);		  //-> close the client socket
	}
	else 
	{ //-> print the received data
		buff[bytes] = '\0';
		std::string message(buff);
	    std::vector<std::string> lines = storeInputLines(client, message);
		if (client.getNeedsCap() == true && client.getCapisSuccess() == false)
		{
			
			for (size_t i = 0; i < lines.size(); i++)
				authenticate(client, lines[i], client.getNeedsCap());
		}
		else if (client.getNeedsCap() == false && client.ifAuthenticated())
		{
			for (size_t i = 0; i < lines.size(); i++)
            	authenticate(client, lines[i]);
		}
		else if (!client.ifAuthenticated())
		{
			for (size_t i = 0; i < lines.size(); i++)
        	{
            const std::string &line = lines[i];

            if (line.rfind("INVITE ", 0) == 0)
                inviteCommand(fd, line);
            else if (line.rfind("KICK ", 0) == 0)
                kickCommand(fd, line);
            else if (line.rfind("TOPIC ", 0) == 0)
                topicCommand(fd, line);
            else if (line.find("QUIT", 0) == 0)
                processQuit(fd, line);
            else if (line.find("JOIN", 0) == 0)
                handleChannel(fd, line);
            else if (line.find("PRIVMSG", 0) == 0)
                processPrivmsg(fd, line);
            else if (line.find("MODE") != std::string::npos)
                handleMode(fd, line);
			}
		}
		else
		{
			std::string buff = "Invalid Command: try again!\n";
			send(fd, buff.c_str(), buff.size(), 0);
			// std::cout << YEL << "Client <" << fd << "> Data: " << WHI << buff;
		} // handling authentication error to be displayed to the client.
	}
}

// void Server::receivePong(int fd)
// {
// 	time_t currentTime = time(NULL); // Get current timestamp
//     std::map<int, time_t>::iterator it = clientLastPing.find(fd);
// 	it->second = currentTime;
// }

void Server::reverseRotate(std::stack<std::string> &s)
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

void Server::processQuit(int fd, const std::string &reason)
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
	std::cout << RED << "Client <" << fd << "> Disconnected" << std::endl;
}

void Server::acceptNewClient()
{
	Client cli; //-> create a new client
	struct sockaddr_in cliadd;
	struct pollfd newPoll;
	socklen_t len = sizeof(cliadd);

	int incofd = accept(serSocketFd, (sockaddr *)&(cliadd), &len); //-> accept the new client
	if (incofd == -1)
	{
		std::cout << "accept() failed" << std::endl;
		return;
	}
	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
	{
		std::cout << "fcntl() failed" << std::endl;
		return;
	}

	newPoll.fd = incofd;	 //-> add the client socket to the pollfd
	newPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	newPoll.revents = 0;	 //-> set the revents to 0

	cli.setFd(incofd);							//-> set the client file descriptor
	cli.setIpAdd(inet_ntoa((cliadd.sin_addr))); //-> convert the ip address to string and set it
	cli.setNeedsCap(false);
	clients.push_back(cli);						//-> add the client to the vector of clients
	fds.push_back(newPoll);						//-> add the client socket to the pollfd
	// clientLastPing[cli.getFd()] = time(NULL);
	// authenticatedClients[incofd] = false;
	std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::serSocket()
{
	int en = 1;
	struct sockaddr_in add;
	struct pollfd newPoll;
	add.sin_family = AF_INET;		  //-> set the address family to ipv4
	add.sin_addr.s_addr = INADDR_ANY; //-> set the address to any local machine address
	add.sin_port = htons(this->port); //-> convert the port to network byte order (big endian)

	serSocketFd = socket(AF_INET, SOCK_STREAM, 0); //-> create the server socket
	if (serSocketFd == -1)						   //-> check if the socket is created
		throw(std::runtime_error("failed to create socket"));
	if (setsockopt(serSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) //-> set the socket option (SO_REUSEADDR) to reuse the address
		throw(std::runtime_error("failed to set option (SO_REUSEADDR) on socket"));
	if (fcntl(serSocketFd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		throw(std::runtime_error("failed to set option (O_NONBLOCK) on socket"));
	if (bind(serSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) //-> bind the socket to the address
		throw(std::runtime_error("failed to bind socket"));
	if (listen(serSocketFd, SOMAXCONN) == -1) //-> listen for incoming connections and making the socket a passive socket
		throw(std::runtime_error("listen() failed"));
	newPoll.fd = serSocketFd; //-> add the server socket to the pollfd
	newPoll.events = POLLIN;  //-> set the event to POLLIN for reading data
	newPoll.revents = 0;	  //-> set the revents to 0
	fds.push_back(newPoll);	  //-> add the server socket to the pollfd
}

void Server::serverInit(int port, std::string pass)
{
	this->port = port;
	this->password = pass;
	serSocket(); //-> create the server socket

	if (!gethostname(this->hostname, sizeof(this->hostname)))
		this->startTime = getCurrentDateTime();
	std::cout << GRE << "Server <" << serSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Listening on " << this->hostname << " on " << this->port << " \r\n";
	while (Server::signal == false)
	{ //-> run the server until the signal is received 

		if ((poll(&fds[0], fds.size(), -1) == -1) && Server::signal == false) //-> wait for an event
			throw(std::runtime_error("poll() failed"));

		for (size_t i = 0; i < fds.size(); i++)
		{ //-> check all file descriptors
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
	std::string capMessage = "CAP * LS :multi-prefix\r\n";
	send(fd, capMessage.c_str(), capMessage.size(), 0);
	return;
}

void Server::processCapReq(int fd, const std::string &message)
{
	std::cout <<  "---" << message << "---" << std::endl;
	if (message.find(":multi-prefix") != std::string::npos)
	{
		std::string capNak = "CAP * ACK :multi-prefix\r\n"; // Acknowledge multi-prefix
		send(fd, capNak.c_str(), capNak.size(), 0);
		return;
	}
	if (message.find("sasl") != std::string::npos)
	{
		std::string capAck = "CAP * NAK :sasl\r\n"; // Reject  SASL
		send(fd, capAck.c_str(), capAck.size(), 0);
	}
}

void Server::validatePassword(Client& client, const std::string &receivedPassword)
{
	std::map<std::string, bool> &aMap = client.getFaceOutheDirt();
	std::map<std::string, bool>::iterator bt = aMap.find("pass");
	if (bt != aMap.end())
	{
		if (bt->second == true)
		{
			std::string errMsg = std::string(RED) + ":" + this->hostname + " 462 " + client.getIPadd() + " :You may not reregister\r\n" + std::string(EN);
			send(client.getFd(), errMsg.c_str(), errMsg.size(), 0);
			return;
		}
	}
	if (receivedPassword.empty())
	{
		std::string errMsg = std::string(RED) + ":" + this->hostname + " 461 " + client.getIPadd() + " PASS :Not enough parameters\r\n" + std::string(EN);
		send(client.getFd(), errMsg.c_str(), errMsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	if (!receivedPassword.compare(this->password))
	{
		bt->second = true;
		return; // Authentication successful
	}
	else
	{
		std::string errMsg = std::string(RED) + ":" + this->hostname + " 464 " + client.getIPadd() + " :Password incorrect\r\n" + std::string(EN);
		send(client.getFd(), errMsg.c_str(), errMsg.size(), 0); // ERR_PASSWDMISMATCH
		return;
	}
	return; // Authentication failed
}

void Server::processUser(Client& client, std::string& username, std::string& ident, std::string& host, std::string& realname)
{
	// Split the message into parts
	std::map<std::string, bool> &aMap = client.getFaceOutheDirt();
	// Check minimum parameter count
	if (ident.empty() || username.empty() || host.empty() || realname.empty() || isValidNickname(username) == false)
	{
		std::string errMsg = std::string(RED) + ":" + this->hostname + " 461 " + client.getIPadd() + " USER :Not enough parameters\r\n" + std::string(EN);
		send(client.getFd(), errMsg.c_str(), errMsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	// Check if the user is already registered
	std::map<std::string, bool>::iterator bt = aMap.find("user");
	if (bt != aMap.end())
	{
		if (bt->second == true)
		{
			std::string errMsg = std::string(RED) + ":" + this->hostname + " 462 " + client.getIPadd() + " :You may not reregister\r\n" + std::string(EN);
			send(client.getFd(), errMsg.c_str(), errMsg.size(), 0);
			return;
		}
	}
	// Register the user
	client.setUserName(username, realname);
	return;
}

void Server::clientWelcomeMSG(int fd, Client &client)
{
	if (fd == 3)
		return ;
	std::ostringstream oss;
	oss << this->hostname;
	std::string buff =  ":" + oss.str() + " 375 " + client.getNickname() + " : \033[1;34m===============================================\033[0m" + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : \033[1;32m          IRC Command List and Format        \033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : \033[1;34m===============================================\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : CAP LS  | \033[1;37m /CAP LS\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : PASS    | \033[1;37m <password>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : NICK    | \033[1;37m <nickname>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : USER    | \033[1;37m <username> <hostname> <servername> <realname>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : INVITE  | \033[1;37m <nickname> <channel>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : KICK    | \033[1;37m <channel> <nickname> [<reason>]\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : TOPIC   | \033[1;37m <channel> [<topic>]\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : CAP REQ | \033[1;37m <capability>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : QUIT    | \033[1;37m [<message>]\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : JOIN    | \033[1;37m <channel> [<key>]\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : PRIVMSG | \033[1;37m <target> <message>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : AUTHENTICATE | \033[1;37m <data>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : CAP END | \033[1;37m /CAP END\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : MODE    | \033[1;37m <target> <mode>\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;37m  Available modes:\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;36m    i\033[0m - \033[1;37mSet/remove Invite-only channel\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;36m    t\033[0m - \033[1;37mSet/remove the restrictions of the TOPIC command to channel operators\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;36m    k\033[0m - \033[1;37mSet/remove the channel key (password)\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;36m    o\033[0m - \033[1;37mGive/take channel operator privilege\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " :\033[1;36m    l\033[0m - \033[1;37mSet/remove the user limit to channel\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : \033[1;34m===============================================\033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : \033[1;32m            End of Command List               \033[0m"  + "\n"
	+ ":" + oss.str() + " 372 " + client.getNickname() + " : \033[1;34m===============================================\033[0m"  + "\n"
	+ ":" + oss.str() + " 376 " + client.getNickname() + ":End of message of the day."  + "\n";
	send(fd, buff.c_str(), buff.size(), 0);
}

void Server::sendWelcome(int fd, Client &client)
{
	// 1. RPL_WELCOME (001)
	std::string welcomeMsg = std::string(YEL) + ":" + this->hostname + " 001 " + client.getNickname() + " :Welcome to the IRC Network, " + client.getNickname() + "!" + client.getUserName() + "@" + client.getIPadd() + "\r\n";
	send(fd, welcomeMsg.c_str(), welcomeMsg.size(), 0);

	// 2. RPL_YOURHOST (002)
	std::string yourHostMsg = std::string(YEL) + ":" + this->hostname + " 002 " + client.getNickname() + " :Your host is irssi (" + this->hostname + "), running version 1.0" + "\r\n";
	send(fd, yourHostMsg.c_str(), yourHostMsg.size(), 0);

	// 3. RPL_CREATED (003)
	std::string createdMsg = std::string(YEL) + ":" + this->hostname + " 003 " + client.getNickname() + " :This server was created " + this->startTime + "\r\n";
	send(fd, createdMsg.c_str(), createdMsg.size(), 0);

	// 4. RPL_MYINFO (004)
	std::string myInfoMsg = std::string(YEL) + ":" + this->hostname + " 004 " + client.getNickname() + " irssi (" + this->hostname + ") v1.0 " + " " + "oiklt[klo]\r\n";
	send(fd, myInfoMsg.c_str(), myInfoMsg.size(), 0);

	// 5. RPL_ISUPPORT (005)
	std::string isupportMsg = std::string(YEL) + ":" + this->hostname + " 005 " + client.getNickname() + " irrsi (" + this->hostname + ") :are supported by this server\r\n";
	isupportMsg += "CHANTYPES=# PREFIX=(o)@ CHANLIMIT=#:100 MODES=5 NETWORK=irssi CASEMAPPING=rfc1459\r\n" + std::string(EN);
	send(fd, isupportMsg.c_str(), isupportMsg.size(), 0);
	std::string rpl_userClient = std::string(YEL) + ":" + this->hostname + " 251 " + client.getNickname() + " :There are 10 users and 3 services on 1 server\r\n" + std::string(EN);
	send(fd, rpl_userClient.c_str(), rpl_userClient.size(), 0);
	std::string rpl_useroper = std::string(YEL) + ":" + this->hostname + " 252 " + client.getNickname() + " 2 :operator(s) online\r\n" + std::string(EN);
	send(fd, rpl_useroper.c_str(), rpl_useroper.size(), 0);
	std::string rpl_userunkown = std::string(YEL) + ":" + this->hostname + " 253 " + client.getNickname() + " 1 :unknown connection(s)\r\n" + std::string(EN); // 253 RPL_LUSERUNKNOWN
	send(fd, rpl_userunkown.c_str(), rpl_userunkown.size(), 0);
	std::string wa = std::string(YEL) + ":" + this->hostname + " 254 " + client.getNickname() + " 5 :channels formed\r\n" + std::string(EN);
	send(fd, wa.c_str(), wa.size(), 0);
	std::string waa = std::string(YEL) + ":" + this->hostname + " 255 " + client.getNickname() + " :I have 1 clients and 1 servers\r\n" + std::string(EN);
	send(fd, waa.c_str(), waa.size(), 0);
	clientWelcomeMSG(client.getFd(), client);
}

void Server::processNickUser(Client& client, const std::string &nickname)
{
	// NICK command
	if (nickname.empty())
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 431 " + client.getIPadd() + " :No nickname given\r\n" + std::string(EN);
		send(client.getFd(), errorMsg.c_str(), errorMsg.size(), 0); // ERR_NONICKNAMEGIVEN
		return;
	}
	if (!isValidNickname(nickname))
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 432 " + client.getIPadd() + " " + nickname + " :Erroneous nickname\r\n" + std::string(EN); // ERR_ERRONEUSNICKNAME
		send(client.getFd(), errorMsg.c_str(), errorMsg.length(), 0);
		return;
	}
	if (nicknameMap.find(nickname) != nicknameMap.end())
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 433 " + client.getIPadd() + " " + nickname + " :Nickname is already in use\r\n" + std::string(EN); // ERR_NICKNAMEINUSE
		send(client.getFd(), errorMsg.c_str(), errorMsg.length(), 0);
		return;
	}
	std::string oldNickname = client.getNickname();
	if (!oldNickname.empty())
		nicknameMap.erase(oldNickname); // Remove old nickname from the map
	client.setNickname(nickname);
	nicknameMap[nickname] = client.getFd();																								   // Add the new nickname to the map
	std::string response = std::string(GRE) + ":" + oldNickname + " NICK " + client.getNickname() + "\r\n" + std::string(EN); // Inform the client of the nickname change
	send(client.getFd(), response.c_str(), response.length(), 0);
	std::cout << "Client <" << client.getFd() << "> changed nickname to: " << nickname << std::endl;
}

std::vector<Client>::iterator Server::getClient(int fd)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->getFd() == fd)
			return it;
	}
	return (clients.end());
}

bool Server::isValidNickname(const std::string &nickname)
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

void Server::handleChannel(int fd, const std::string &message)
{
	std::vector<Client>::iterator it = getClient(fd);
	if (it == clients.end())
		throw std::runtime_error("No client was found\n");
	Client &client = (*this)[it];
	// Extract parameters after JOIN , client is going to send JOIN #channel1,#channel2 key1,key2 or JOIN #channel1
	size_t paramsStart = message.find(' ') + 1;
	if (paramsStart == std::string::npos || paramsStart >= message.length())
	{
		std::string errormsg = std::string(RED) + ":" + this->hostname + " 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n" + std::string(EN);
		send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
		return;
	}
	std::string params = message.substr(paramsStart);
	// Split channels and keys
	size_t spacePos = params.find(' ');
	std::string channelsPart = params.substr(0, spacePos);									   // Comma-separated channel names
	std::string keysPart = (spacePos != std::string::npos) ? params.substr(spacePos + 1) : ""; // Comma-separated keys
	// Parse channels and keys
	std::vector<std::string> channels = splitByDelimiter(channelsPart, ',');
	std::vector<std::string> keys = splitByDelimiter(keysPart, ',');

	// Iterate through each channel and call joinChannel
	for (size_t i = 0; i < channels.size(); ++i)
	{
		const std::string &channelName = channels[i];
		const std::string &key = (i < keys.size()) ? keys[i] : ""; // Match keys to channels if possible
		if (!isValidChannelName(channelName))
		{
			std::string errormsg = std::string(RED) + ":" + this->hostname + " 476 " + channelName + " :Bad Channel Mask\r\n";
			send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_BADCHANMASK
			continue;
		}
		joinChannel(fd, channelName, key);
	}
}

/*Initial parsing is done and we're joining a channel*/
void Server::joinChannel(int fd, const std::string &channelName, const std::string &key)
{
	std::vector<Client>::iterator iter = getClient(fd);
	if (iter == clients.end())
		throw std::runtime_error("Error finding client\n");
	Client &client = (*this)[iter];
	std::map<std::string, Channel>::iterator it = channels.find(channelName);
	if (it == channels.end())
	{
		// Channel doesn't exist, so create it
		channels[channelName] = Channel(channelName, key, client.getFd());
		it = channels.find(channelName); // Re-get the iterator after creation
	}
	Channel &channel = it->second;
	if (channel.isInChannel(fd))
		return;
	if (channel.isInviteOnly() && !channel.isInvitedUser(fd))
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i)" + std::string(EN) + "\r\n";
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
	}
	if (channel.isFull())
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l)" + std::string(EN) + "\r\n";
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
	}
	if (!channel.getKey().empty() && channel.getKey() != key)
	{
		std::string errorMsg = std::string(RED) + ":" + this->hostname + " 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k)" + std::string(WHI) + "\r\n";
		send(fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
	}

	// 4. Add the client to the channel
	channel.addClient(fd);
	std::cout << channel.getKey() << std::endl;
	// 5. Broadcast JOIN message to all clients in the channel
	std::string joinMessage = ":" + client.getNickname() + " JOIN :" + channelName + "\r\n" + std::string(EN);
	channel.broadcastToChannel(joinMessage);
	// channel.removeClientFromInvitation(fd);
	// 6. Send the channel topic (or indicate no topic set)
	if (!channel.getTopic().empty())
	{
		std::string info = std::string(YEL) + ":" + this->hostname + " 332 " + client.getNickname() + " " + channelName + " :" + channel.getTopic() + std::string(EN) + "\r\n";
		send(fd, info.c_str(), info.size(), 0);
	}
	else
	{
		std::string info = std::string(YEL) + ":" + this->hostname + " 331 " + client.getNickname() + " " + channelName + " :No topic is set" + std::string(EN) + "\r\n";
		send(fd, info.c_str(), info.size(), 0);
	}

	// 7. Send the list of users in the channel
	std::vector<int> clientList = channel.listUsers();
	for (std::vector<int>::iterator it = clientList.begin(); it != clientList.end(); ++it)
	{
		std::vector<Client>::iterator bt = getClient(*it);
		if (bt == clients.end())
			throw std::runtime_error("Error finding clients\n");
		Client &user = (*this)[bt];
		std::string msg = std::string(YEL) + ":" + this->hostname + " 353 " + client.getNickname() + " = " + channelName + " :" + (channel.isOperator(user.getFd()) ? "@" : "") + user.getNickname() + std::string(EN) + "\r\n";
		send(fd, msg.c_str(), msg.size(), 0);
	}
	std::string msg = std::string(YEL) + ":" + this->hostname + " 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list" + std::string(EN) + "\r\n";
	send(fd, msg.c_str(), msg.size(), 0);
}

std::vector<std::string> Server::splitByDelimiter(const std::string &str, char delimiter)
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

bool Server::isValidChannelName(const std::string &channelName)
{
	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
		return false;
	// Additional validation rules can be added here
	return true;
}

Client &Server::operator[](std::vector<Client>::iterator it)
{
	if (it == clients.end())
		throw std::out_of_range("Iterator out of range for clients vector");
	return *it;
}

void Server::processPrivmsg(int fd, const std::string &message)
{
	std::vector<Client>::iterator bt = getClient(fd);
	if (bt == clients.end())
		throw std::runtime_error("Error finding client\n");
	Client &sender = (*this)[bt];
	size_t commandEnd = message.find(' ');
	// if (commandEnd == std::string::npos || message.substr(0, commandEnd) != "PRIVMSG") {
	//     std::string error = std::string(RED) + "421: Unknown command\r\n" + std::string(WHI);
	// }
	// Skip the "PRIVMSG" part
	size_t targetStart = commandEnd + 1;			  // Position after "PRIVMSG "
	size_t spacePos = message.find(' ', targetStart); // Find space after target
	if (spacePos == std::string::npos)
	{
		// If there's no space after the target, no message text is provided
		std::string error = std::string(RED) + ":" + this->hostname + " 411 " + sender.getNickname() + " :No recipient given (PRIVMSG)\r\n" + std::string(EN);
		send(fd, error.c_str(), error.size(), 0); // ERR_NORECIPIENT
		return;
	}
	// Extract the target
	std::string target = message.substr(targetStart, spacePos - targetStart);
	// Skip any spaces after the target and check for message text
	size_t textStart = message.find_first_not_of(' ', spacePos + 1);
	if (textStart == std::string::npos)
	{
		// If no text is found after the target, return an error
		std::string error = std::string(RED) + ":" + this->hostname + " 412 " + sender.getNickname() + " :No text to send\r\n" + std::string(EN);
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
			std::string error = std::string(RED) + ":" + this->hostname + " 404 " + sender.getNickname() + " " + target + " :Cannot send to channel\r\n" + std::string(EN);
			send(fd, error.c_str(), error.size(), 0); // ERR_CANNOTSENDTOCHAN
			return;
		}
		// Check if the user is banned or not allowed in the channel
		Channel &channel = it->second;
		if (!channel.isInChannel(fd) || channel.isBanned(sender.getNickname()))
		{
			std::string error = std::string(RED) + ":" + this->hostname + " 404 " + sender.getNickname() + " " + target + " :Cannot send to channel\r\n" + std::string(EN);
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
			std::string error = std::string(RED) + ":" + this->hostname + " 401 " + sender.getNickname() + " " + target + " :No such nick/channel\r\n" + std::string(EN);
			send(fd, error.c_str(), error.size(), 0); // ERR_NOSUCHNICK
			return;
		}
		Client &recepient = (*this)[ct];
		// Send the private message to the user
		std::string response = ":" + sender.getNickname() + " PRIVMSG " + recepient.getNickname() + " :" + text + "\r\n";
		send(recepient.getFd(), response.c_str(), response.size(), 0);
	}
}

// // Helper methods for getting client and checking channels
std::vector<Client>::iterator Server::getClientUsingNickname(const std::string &nickname)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		std::string clientsNick = it->getNickname();
		if (!clientsNick.compare(nickname))
			return it;
	}
	return (clients.end());
}

std::string Server::trim(const std::string &str)
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
