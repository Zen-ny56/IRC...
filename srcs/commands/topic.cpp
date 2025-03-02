#include "../../include/Server.hpp"
#include "../../include/Channel.hpp"

void Server::topicCommand(int fd, std::string const &message)
{
    std::vector<Client>::iterator it = getClient(fd);
    if (it == clients.end())
    {
        throw std::runtime_error("Client Not Found");
    }
    Client &client = *it;

    // Ensure message has enough content
    if (message.size() < 7) 
    {
        std::string err = "461 " + client.getNickname() + " TOPIC :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    // Extract channel name and topic from the message
    std::string trimmed = message.substr(6); // Remove "TOPIC " prefix
    std::istringstream stream(trimmed);
    
    std::string channelName;
    stream >> channelName; // Extract channel name

    // Ensure channel name starts with '#' or '&' (IRC standard)
    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
    {
        std::string err = "403 " + client.getNickname() + " :Invalid channel name\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    // Extract topic, if provided
    std::string topic;
    if (stream.peek() == ' ') 
    {
        stream.get(); // Consume leading space
    }
    std::getline(stream, topic);
    
    // Remove leading ':' if present (standard IRC topic format)
    if (!topic.empty() && topic[0] == ':') 
    {
        topic = topic.substr(1);
    }
    
    // Check if the channel exists
    std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);
    if (channelIt == channels.end())
    {
        std::string err = "403 " + client.getNickname() + " " + channelName + " :No such channel\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }
    Channel &channel = channelIt->second;

    // Check if the client is in the channel
    if (!channel.isInChannel(fd))
    {
        std::string err = "442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    // If no topic is provided, return the curt topic
    if (topic.empty())
    {
        if (channel.getTopic().empty())
        {
            std::string rpl_notopic = "331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n";
            send(fd, rpl_notopic.c_str(), rpl_notopic.size(), 0);
        }
        else
        {
            std::string rpl_topic = "332 " + client.getNickname() + " " + channelName + " :" + channel.getTopic() + "\r\n";
            send(fd, rpl_topic.c_str(), rpl_topic.size(), 0);
        }

        std::ostringstream oss;
        oss << time(NULL);
        std::string rpl_whotime = "333 " + client.getNickname() + " " + channelName + " " + client.getNickname() + " " + oss.str() + "\r\n";
        send(fd, rpl_whotime.c_str(), rpl_whotime.size(), 0);
        return;
    }

    // If topic is the same as the current one, still notify the channel
    if (channel.getTopic() == topic)
    {
        std::string topicChangedMessage = "TOPIC " + channelName + " TOPIC " + " :" + topic + "\r\n";
        channel.broadcastToChannel(topicChangedMessage);
        return;
    }

    // Set the new topic
    channel.setTopic(topic);

    // Broadcast topic change to all clients in the channel
    std::string topicChangedMessage = "TOPIC " + channelName + " :" + topic + "\r\n";
    channel.broadcastToChannel(topicChangedMessage);

    // Notify client about the topic update
    std::string rpl_topicset = "332 " + client.getNickname() + " " + channelName + " :" + topic + "\r\n";
    send(fd, rpl_topicset.c_str(), rpl_topicset.size(), 0);
}
