#include "../../include/Server.hpp"
#include "../../include/Channel.hpp"

void Server::topicCommand(int fd, const std::string &message)
{
    if (message.find("TOPIC ") != 0) // Ensure the command starts with "TOPIC "
        return;

    std::vector<Client>::iterator it = getClient(fd);
    if (it == clients.end())
        throw std::runtime_error("Client was not found\n");

    Client &client = *it;

    // Remove "TOPIC " prefix
    std::string trimmedMessage = message.substr(6); // Correct offset

    if (trimmedMessage.size() == 0)
    {
        std::string errorMsg = std::string(RED) + " Usage: TOPIC <#channel> [topic]" + std::string(EN);
        send(fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    std::istringstream iss(trimmedMessage);
    std::string channelName;
    iss >> channelName;

    std::string topic;
    std::getline(iss, topic);

    if (topic.size() > 0 && topic[0] == ' ')
    {
        topic = topic.substr(1); // Remove leading space from topic
    }

    std::map<std::string, Channel>::iterator channelIt = isValidChannelName(channelName);
    if (channelIt == channels.end())
    {
        std::string errorMsg = std::string(RED) + " Channel not found" + std::string(EN);
        send(fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    Channel &channel = channelIt->second;

    if (!channel.isInChannel(client.getFd()))
    {
        std::string errorMsg = std::string(RED) + " You're not in this channel" + std::string(EN);
        send(fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    if (topic.size() == 0)
    {
        // If no topic provided, return the current topic
        std::string response;
        if (channel.getTopic().size() == 0)
        {
            response = "No topic is set.";
        }
        else
        {
            response = "Current topic: " + channel.getTopic();
        }
        send(fd, response.c_str(), response.size(), 0);
        return;
    }

    if (!channel.isOperator(client.getFd()))
    {
        std::string errorMsg = std::string(RED) + " You don't have permission to set the topic" + std::string(EN);
        send(fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    // Set the new topic
    channel.setTopic(topic);
    std::string successMsg = "Topic for " + channelName + " set to: " + topic;
    send(fd, successMsg.c_str(), successMsg.size(), 0);
}
