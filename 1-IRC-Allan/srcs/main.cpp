#include "../include/Client.hpp"
#include "../include/Server.hpp"

int main(int ac, char **av)
{
	if (ac != 3){
		std::cout << RED << "./ircserv port password" << std::endl;
		return (1);}
	std::string port_check = av[1];
	int port = std::atoi(av[1]);
	// Compare the integer value to 1024
	if (port <= 1024 || port_check.empty()){
		std::cout << RED << "Enter valid input" << std::endl;
		return (1);}
	 std::string pass = av[2];
    // Check if the string is empty
    if (pass.empty()){
		std::cout << RED <<  "The second argument is empty." << std::endl;
		return (1);}
	Server ser;
	std::cout << "---- SERVER ----" << std::endl;
	try{
		signal(SIGINT, Server::signalHandler); //-> catch the signal (ctrl + c)
		signal(SIGQUIT, Server::signalHandler); //-> catch the signal (ctrl + \)
		ser.serverInit(port, pass); //-> initialize the server
	}
	catch(const std::exception& e){
		ser.closeFds(); //-> close the file descriptors
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}