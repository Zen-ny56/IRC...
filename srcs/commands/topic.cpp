#include "../../include/Server.hpp"
#include "../../include/Channel.hpp"

void Server::topicCommand(int fd, std::string const &message)
{
    std::vector<Client>::iterator it = getClient(fd);
    if (it == clients.end())
        throw std::runtime_error("Client Not Found");
    Client &client = *it;

    if (message.size() < 7)
    {
        std::string err = std::string(RED) + ":" + this->hostname + " 461 " + client.getNickname() + " TOPIC :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    std::string trimmed = message.substr(6);
    std::istringstream stream(trimmed);

    std::string channelName;
    stream >> channelName; // Extract channel name

    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
    {
        std::string err = std::string(RED) + ":" + this->hostname + " 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    std::string topic;
    if (stream.peek() == ' ')
        stream.get();
    std::getline(stream, topic);

    if (!topic.empty() && topic[0] == ':')
        topic = topic.substr(1);

    std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);
    if (channelIt == channels.end())
    {
        std::string err = std::string(RED) + ":" + this->hostname + " 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }
    Channel &channel = channelIt->second;

    if (!channel.isInChannel(fd))
    {
        std::string err = std::string(RED) + ":" + this->hostname +  " 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    if (topic.empty())
    {
        if (channel.getTopic().empty())
        {
            std::string rpl_notopic = std::string(YEL) + ":" + this->hostname + " 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n";
            send(fd, rpl_notopic.c_str(), rpl_notopic.size(), 0);
        }
        else
        {
            std::string rpl_topic = std::string(YEL) + ":" + this->hostname + " 332 " + client.getNickname() + " " + channelName + " :" + channel.getTopic() + "\r\n";
            send(fd, rpl_topic.c_str(), rpl_topic.size(), 0);
        }

        std::ostringstream oss;
        oss << time(NULL);
        std::string rpl_whotime = std::string(YEL) + ":" + this->hostname + " 333 " + client.getNickname() + " " + channelName + " " + client.getNickname() + " " + oss.str() + "\r\n";
        send(fd, rpl_whotime.c_str(), rpl_whotime.size(), 0);
        return;
    }

    if (channel.getTopic() == topic)
    {
        std::string topicChangedMessage = ":" + client.getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
        channel.broadcastToChannel(topicChangedMessage);
        return;
    }
    if (channel.getTopRes() && !channel.isOperator(fd))
    {
        std::string rpl_topic = std::string(RED) + ":" + this->hostname + " 473 " + client.getNickname() + " " + channelName  + ": Cannot change topic: Topic is locked." +std::string(EN) + "\r\n";
        send(fd, rpl_topic.c_str(), rpl_topic.size(), 0);
        return ;
    }
    channel.setTopic(topic);

    std::string topicChangedMessage = ":" + client.getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
    channel.broadcastToChannel(topicChangedMessage);
    std::string rpl_topicset = std::string(YEL) + ":" + this->hostname + " 332 " + client.getNickname() + " " + channelName + " :" + topic + "\r\n";
    send(fd, rpl_topicset.c_str(), rpl_topicset.size(), 0);
}
