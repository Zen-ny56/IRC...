#include "../../include/Server.hpp"
#include "../../include/Channel.hpp"
#include <cstdio>
void Server::kickCommand(int fd, const std::string &message)
{
    if (message.rfind("/KICK ", 0) == 0)
    {
        std::vector<Client>::iterator it = getClient(fd);
        if (it == clients.end())
            throw std::runtime_error("Client was not found\n");
        Client &client = (*this)[it];

        std::string trimmedMessage = message.substr(5);

        std::istringstream iss(trimmedMessage);
        std::string channelName, user, comment;

        iss >> channelName;
        if (channelName.empty())
        {
            std::string errormsg = std::string(RED) + "461 KICK :Not enough parameters\r\n" + std::string(EN);
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
            return;
        }

        std::vector<std::string> users;
        std::string temp;
        while (iss >> temp)
        {
            std::size_t commaPos = temp.find(',');
            if (commaPos != std::string::npos)
            {
                users.push_back(temp.substr(0, commaPos));
                temp = temp.substr(commaPos + 1);
            }
            if (!temp.empty())
                users.push_back(temp);
        }

        if (iss.peek() != EOF)
        {
            std::getline(iss, comment);
            comment = trim(comment);
        }

        if (users.empty())
        {
            std::string errormsg = std::string(RED) + "461 KICK :Not enough parameters\r\n" + std::string(EN);
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NEEDMOREPARAMS
            return;
        }

        std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);

        if (channelIt == channels.end())
        {
            std::string errormsg = std::string(RED) + "403 " + channelName + " :No such channel\r\n" + std::string(EN);
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NOSUCHCHANNEL
            return;
        }

        Channel &channel = channelIt->second;

        if (!channel.isOperator(fd))
        {
            std::string errormsg = std::string(RED) + "482 " + channelName + " :You're not channel operator\r\n" + std::string(EN);
            send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_CHANOPRIVSNEEDED
            return;
        }

        for (std::vector<std::string>::iterator userIt = users.begin(); userIt != users.end(); ++userIt)
        {
            // Find the target client by nickname
            std::vector<Client>::iterator targetIt = getClientUsingNickname(*userIt);
            if (targetIt == clients.end())
            {
                std::string errormsg = std::string(RED) + "401 " + *userIt + " :No such user\r\n" + std::string(EN);
                send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_NOSUCHNICK
                continue;
            }

            Client &target = *targetIt;

            if (!channel.isInChannel(target.getFd()))
            {
                std::string errormsg = std::string(RED) + "441 " + *userIt + " " + channelName + " :They aren't on that channel\r\n" + std::string(EN);
                send(fd, errormsg.c_str(), errormsg.size(), 0); // ERR_USERNOTINCHANNEL
                continue;
            }

            std::string kickMessage = ":" + client.getNickname() + " KICKED " + channelName + " " + *userIt;
            if (!comment.empty())
            {
                kickMessage += " :" + comment;
            }
            kickMessage += "\r\n";
            channel.broadcastToChannel(kickMessage);
            channel.removeClient(target.getFd());
        }
    }
}
