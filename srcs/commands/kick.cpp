#include "../../include/Server.hpp"
#include "../../include/Channel.hpp"

void Server::kickCommand(int fd, const std::string &message)
{
    // Step 1: Ensure the message starts with "KICK " and remove the prefix
    if (message.rfind("KICK ", 0) == 0)
    {
        std::vector<Client>::iterator it = getClient(fd);
        if (it == clients.end())
            throw std::runtime_error("Client was not found\n");
        Client &client = (*this)[it];

        // Remove the "KICK " prefix to avoid confusion with channel name
        std::string trimmedMessage = message.substr(5);

        // Step 2: Parse the remaining message
        std::istringstream iss(trimmedMessage); // Now start parsing after "KICK "
        std::string channelName, user, comment;

        // Try to extract the channel name
        iss >> channelName;
        if (channelName.empty())
        {
            std::string errormsg = std::string(RED) + "461 KICK :Not enough parameters\r\n";
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
            return;
        }

        // Step 3: Parse users (comma-separated)
        std::vector<std::string> users;
        std::string temp;
        while (iss >> temp)
        {
            // Split by commas if there are multiple users
            std::size_t commaPos = temp.find(',');
            if (commaPos != std::string::npos)
            {
                // Add the part before the comma as a user
                users.push_back(temp.substr(0, commaPos));

                // Process the part after the comma
                temp = temp.substr(commaPos + 1);
            }

            // If there's any remaining part of the user (after comma split), add it
            if (!temp.empty())
                users.push_back(temp);
        }

        // Step 4: Extract the optional comment (if present)
        if (iss.peek() != EOF)
        {
            std::getline(iss, comment);
            comment = trim(comment); // Clean up the comment
        }

        // Check if there are enough parameters (at least one user)
        if (users.empty())
        {
            std::string errormsg = std::string(RED) + "461 KICK :Not enough parameters\r\n";
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
            return;
        }

        // Find the channel
        std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);

        if (channelIt == channels.end())
        {
            std::cout << "Channel not found!" << std::endl; // Debugging log
            std::string errormsg = std::string(RED) + "403 " + channelName + " :No such channel\r\n" + std::string(EN);
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NOSUCHCHANNEL
            return;
        }

        Channel &channel = channelIt->second;

        // Step 5: Check if the client is an operator in the channel
        if (!channel.isOperator(fd))
        {
            std::string errormsg = std::string(RED) + "482 " + channelName + " :You're not channel operator\r\n";
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_CHANOPRIVSNEEDED
            return;
        }

        // Step 6: Iterate over the list of users to kick
        for (std::vector<std::string>::iterator userIt = users.begin(); userIt != users.end(); ++userIt)
        {
            // Find the target client by nickname
            std::vector<Client>::iterator targetIt = getClientUsingNickname(*userIt);
            if (targetIt == clients.end())
            {
                std::string errormsg = std::string(RED) + "401 " + *userIt + " :No such user\r\n";
                send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NOSUCHNICK
                continue;
            }

            Client &target = *targetIt;

            // Step 7: Check if the client is part of the channel
            if (!channel.isInChannel(target.getFd()))
            {
                std::string errormsg = std::string(RED) + "441 " + *userIt + " " + channelName + " :They aren't on that channel\r\n";
                send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_USERNOTINCHANNEL
                continue;
            }

            // Step 8: Send kick message to the channel
            std::string kickMessage = ":" + client.getNickname() + " KICK " + channelName + " " + *userIt;
            if (!comment.empty())
            {
                kickMessage += " :" + comment;
            }
            kickMessage += "\r\n";
            channel.broadcastToChannel(kickMessage);

            // Step 9: Remove the target from the channel
            channel.removeClient(target.getFd());
            target.setChannelName(""); // Remove client from the channel
        }
    }
}
