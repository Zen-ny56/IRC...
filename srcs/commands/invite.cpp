#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include "../../include/Channel.hpp"

void Server::inviteCommand(int fd, std::string const &message)
{
    if (message.rfind("INVITE ") == 0)
    {
        std::vector<Client>::iterator it = getClient(fd);
        if (it == clients.end())
            throw std::runtime_error("Client Not Found");
        Client &client = *it;
        std::string trimmed = message.substr(7);
        std::istringstream re(trimmed);
        std::string nickName, channelName;
        re >> nickName >> channelName;

        if (nickName.empty() || channelName.empty())
        {
            std::string err = std::string(RED) + ":" + this->hostname + " 461 " + client.getNickname() +  " INVITE :Not enough parameters" + std::string(EN)+ "\r\n";
            send(fd, err.c_str(), err.size(), 0);
            return;
        }

        std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);
        if (channelIt == channels.end() || channelIt->second.listUsers().empty())
        {
            std::string err = std::string(RED) + ":" + this->hostname  + " 403 " + client.getNickname() + " " + channelName + " :No such channel" + std::string(EN)+ "\r\n";
            send(fd, err.c_str(), err.size(), 0);
            return;
        }
        Channel &channel = channelIt->second;

        if (channel.isInChannel(fd) == 0)
        {
            std::string err = std::string(RED) + ":" + this->hostname  + " 442 " +  client.getNickname() + " " + channelName + " :You're not on that channel" + std::string(EN)+ "\r\n";
            send(fd, err.c_str(), err.size(), 0);
            return;
        }

        if (channel.isInviteOnly() && !channel.isOperator(fd))
        {
            std::string err = std::string(RED) + ":" + this->hostname  + " 473 " + channelName + " :Cannot join channel (+i)" + std::string(EN)+ "\r\n";
            send(fd, err.c_str(), err.size(), 0);
            return;
        }
        std::vector<Client>::iterator targetClientIt = getClientUsingNickname(nickName);
        if (targetClientIt != clients.end())
        {
            int targetFd = targetClientIt->getFd();
            if (channel.isInChannel(targetFd))
            {
                std::string err = std::string(RED) + ":" + this->hostname + " 443 " + client.getNickname() + " " + nickName + " " + channelName + " :User already on channel" + std::string(EN)+ "\r\n";
                send(fd, err.c_str(), err.size(), 0);
                return;
            }
            std::string rpl_inviting = std::string(GRE) + ":" + this->hostname + " 341 " + client.getNickname() + " INVITED " + nickName + " to " + channelName  + std::string(EN) + "\r\n";
            send(fd, rpl_inviting.c_str(), rpl_inviting.size(), 0);
            std::string inviteMessage = ":" + client.getNickname() + "!~" + this->hostname + " INVITE " + nickName + " " + channelName  + std::string(EN) + "\r\n";
            send(targetFd, inviteMessage.c_str(), inviteMessage.size(), 0);
            channel.addToInvitation(targetFd);
        }
    }
}
