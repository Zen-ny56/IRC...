#include "../include/Client.hpp"
#include "../include/Server.hpp"


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
//check for empty port string
//check for anything that isnt a digit
// convert the end value to int with Atoi and set with int &port
//Values for port being > 1024 and <= 65535 (need to check)

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
//Check for arg count
//Check port Value with function isValidPort above
//check if pass is empty, without checking only num
// because pass CAN contain other chars :)
//------------------------------------------------
// Parsing checks passed working:
// ./ircserv	Error: Usage message
// ./ircserv 12345	Error: Usage message
// ./ircserv abc pass	Error: Invalid port
// ./ircserv 70000 pass	Error: Invalid port
// ./ircserv 1023 pass	Error: Invalid port
// ./ircserv 1234 ""	Error: Password cannot be empty
// ./ircserv 6667 pass	---- SERVER ---- (Starts the server)

bool quotes_uneven(const std::string &message) {
    size_t quotes_count = std::count(message.begin(), message.end(), '\"');
	bool quotes_even = (quotes_count % 2 == 0);

	if (quotes_even != 0)
	{
		std::cout << "Even number of quotes detected in message." << std::endl;
		return (0);
	}
	else 
	{
		std::cout << "Odd number of quotes detected in message." << std::endl;
		return (1);
	}
}
// Return Type:
//returns 0 if the quote count is even
//returns 1 if the quote count is not even

//Uneven Quotes trigger an error response, preventing exectuion of message
	//This is typically an ERR_NEEDMOREPARAMS (461) or ERR_UNKNOWNCOMMAND (421), depending on the command.
	//--- Needs to check the code of error.

//Todos:
// Modify quotes_uneven() to properly reject messages and send errors.
// Implement process_escaped_quotes() to correctly handle \".
// Ensure proper error messages (ERR_NEEDMOREPARAMS) are returned to IRSSI.
// Test the behavior with IRSSI client by sending:
// Messages with even quotes ✅
// Messages with odd quotes ❌
// Messages with escaped quotes (\") ✅
// This will ensure your ft_irc server correctly handles quotes in line with IRC protocols! 🚀



int main(int ac, char **av)
{
    int port;
    std::string pass;

	std::cout << "TESTING MODE" << std::endl;
	if (!quotes_uneven("Hello\"")){
    // send_error(client_fd, "Missing quote detected in message.");
	}
	return (0);
    //-------------

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
    {	ser.closeFds(); // Close open file descriptors
        std::cerr << e.what() << std::endl;
	}
    std::cout << "The Server Closed!" << std::endl;
}

// // Old int main for reference
// int main(int ac, char **av)
// {
// 	if (ac != 3){
// 		std::cout << RED << "./ircserv port password" << std::endl;
// 		return (1);}

// 	std::string port_check = av[1];
// 	int port = std::atoi(av[1]);

// 	// Compare the integer value to 1024
// 	if (port <= 1024 || port_check.empty()){
// 		std::cout << RED << "Error: Invalid port value." << std::endl;
// 		std::cout << RED << "Port Value Must be Above 1024!." << std::endl;
// 		return (1);}
// 	 std::string pass = av[2];
//     // Check if the string is empty
//     if (pass.empty()){
// 		std::cout << RED <<  "The second argument is empty." << std::endl;
// 		return (1);}
// 	Server ser;
// 	std::cout << "---- SERVER ----" << std::endl;
// 	try{
// 		signal(SIGINT, Server::signalHandler); //-> catch the signal (ctrl + c)
// 		signal(SIGQUIT, Server::signalHandler); //-> catch the signal (ctrl + \)
// 		ser.serverInit(port, pass); //-> initialize the server
// 	}
// 	catch(const std::exception& e){
// 		ser.closeFds(); //-> close the file descriptors
// 		std::cerr << e.what() << std::endl;
// 	}
// 	std::cout << "The Server Closed!" << std::endl;
// }