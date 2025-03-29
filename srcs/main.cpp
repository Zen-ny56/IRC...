#include "../include/Client.hpp"
#include "../include/Server.hpp"

std::string getCurrentDateTime()
{
    // Get current time as time_t
    std::time_t now = std::time(0);
    // Convert to local time (returns pointer to static internal structure)
    struct std::tm* localTime = std::localtime(&now);
    // Buffer to hold formatted date/time string
    char buffer[80];
    // Format: "YYYY-MM-DD HH:MM:SS"
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

    return std::string(buffer);
}

bool isValidPort(const std::string &port_str, int &port)
{
    if (port_str.empty())
        return false;
    for (size_t i = 0; i < port_str.length(); i++)
    {
        if (!isdigit(port_str[i]))
            return false;
    }
    port = std::atoi(port_str.c_str());
    return (port > 1024 && port <= 65535);
}

bool parseArguments(int ac, char **av, int &port, std::string &pass)
{
    if (ac != 3)
    {
        std::cerr << RED << "Error: Usage: ./ircserv <port> <password>" << std::endl << EN;
		return false;
    }
    std::string port_str = av[1];
    pass = av[2];
    if (!isValidPort(port_str, port))
    {
        std::cerr << RED << "Error: Invalid port. Port must be numeric and between 1025 and 65535." << std::endl << EN;
        return false;
    }
    if (pass.empty())
    {
        std::cerr << RED << "Error: Password cannot be empty." << std::endl << EN;
        return false;
    }
    return true;
}

int main(int ac, char **av)
{
    int port;
    std::string pass;

    if (!parseArguments(ac, av, port, pass))
        return 1;
    Server ser;
    std::cout << "---- SERVER ----" << std::endl;
    try
    {	signal(SIGINT, Server::signalHandler);  // Handle Ctrl+C
        signal(SIGQUIT, Server::signalHandler); // Handle Ctrl+"\"
        ser.serverInit(port, pass); /*Initialize the server*/
	}
    catch (const std::exception &e)
    {
        ser.closeFds(); // Close open file descriptors
        std::cerr << e.what() << std::endl;
	}
    std::cout << "The Server Closed!" << std::endl;
}