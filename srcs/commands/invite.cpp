#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include "../../include/Channel.hpp"

void Server::inviteCommand(int fd, std::string const &message)
{
    // Parse the message
    if (message.rfind("/INVITE ") == 0)
	{
        std::vector<Client>::iterator it = getClient(fd);
        if (it == clients.end())
            throw std::runtime_error("Client Not Found");
        Client &client = *it;

        // Trim and extract nickname and channel name
        std::string trimmed = message.substr(7);
        std::istringstream re(trimmed);
        std::string nickName, channelName;
        re >> nickName >> channelName;

        // Check if parameters are valid
        if (nickName.empty() || channelName.empty()) {
            std::string err = std::string(RED) + "461 :Not enough parameters" + std::string(EN);
            send(fd, err.c_str(), err.size(), 0);
            return;
        }

        // Check if channel exists and has at least one member
        std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);
        if (channelIt == channels.end() || channelIt->second.listUsers().empty()) {
            std::string err = std::string(RED) + "403 " + channelName + " :No such channel" + std::string(EN);
            send(fd, err.c_str(), err.size(), 0);
            return;
        }
        Channel &channel = channelIt->second;

        // Check if client is a member of the channel
        if (channel.isInChannel(fd) == 0)
		{
            std::string err = std::string(RED) + "442 " + channelName + " :You're not on that channel" + std::string(EN);
            send(fd, err.c_str(), err.size(), 0);
            return;
        }

        // If the channel is invite-only, check if the client is an operator
        if (channel.isInviteOnly() && !channel.isOperator(fd))
		{
            std::string err = std::string(RED) + "473 " + channelName + " :Cannot invite to channel (Invite-only channel)" + std::string(EN);
            send(fd, err.c_str(), err.size(), 0);
            return;
        }
        // Check if the target user is already in the channel 
        std::vector<Client>::iterator targetClientIt = getClientUsingNickname(nickName);
        if (targetClientIt != clients.end())
		{
			int targetFd = targetClientIt->getFd();
			if (channel.isInChannel(targetFd))
			{
				std::string err = std::string(RED) + "443 " + nickName + " " + channelName + " :User already on channel" + std::string(EN);
				send(fd, err.c_str(), err.size(), 0);
				return;
			}
			// Send RPL_INVITING response to the inviter
			std::string rpl_inviting = "341 " + client.getNickname() + " INVITED " + nickName + " to " + channelName + "\r\n";
			send(fd, rpl_inviting.c_str(), rpl_inviting.size(), 0);
        	// Send INVITE message to the target user
			std::string inviteMessage =  nickName + " INVITED " + " to " + channelName + "\r\n";
            send(targetFd, inviteMessage.c_str(), inviteMessage.size(), 0);
            // channel.addToInvitation(targetFd);
		}
    }
}
