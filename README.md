🌐 IRC Server Project

Collaborative project by Naib & Allan
Built on top of irc.freenode.net — an Internet Relay Chat server implementation in modern C++

📖 Overview

We built a fully functioning IRC (Internet Relay Chat) server, compatible with popular IRC clients like irssi and netcat (nc). The server handles connections, authentication, command parsing, channel management, and message routing — all while following IRC protocol standards.

🔧 How Did We Implement This?

1️⃣ Class Structure & Server Architecture

🧩 All other classes are part of the main Server class

  👥 Clients are stored as:
     std::vector<Client> clients;
  
  🛰️ Channels are stored as:
    std::map<std::string, Channel> channels;

2️⃣ Setting Up the Server

🧱 Sockets & Non-Blocking I/O

We used the socket() system call to create a socket.

It was set to non-blocking mode using fcntl() and then bound to a specific address and port using bind().
  
🔁 Handling Connections

We used poll() for I/O multiplexing:

A std::vector<struct pollfd> holds all active file descriptors.

The first element listens for incoming connections (via accept()).

All other elements represent client connections.

🤝 Accepting New Clients

Upon a new connection:

We accept() the client.

A new Client object is created.

It's added to the clients vector.

3️⃣ Authentication

🔐 Capability Negotiations

Our authentication process supports both manual nc connections and automated irssi clients.

We overloaded the authenticate() function to handle both workflows:

For nc users:

PASS mypass

NICK mynick

USER myuser

For irssi, we handled capability negotiations where the client sends PASS/NICK/USER automatically.

4️⃣ Channel Functionality

🔗 JOIN Command

On receiving JOIN #channel:

If the channel doesn’t exist in our map, it gets created:

std::map<std::string, Channel> channels;

The user is added to the channel's user list.

⚙️ MODE Command

Parsing Modes with Parameters:

We used two stacks to parse and pair modes with their parameters:

std::stack<std::string> modewitParams;

std::stack<std::string> params;

Then we reverse rotate and pop() items from both stacks.

Parsed modes and their arguments are stored in a map for that's an attribute for the Channel class:

  std::map<std::string, std::string> modes;

🚀 Getting Started

Compile & Run

make

./ircserv <port> <password>

Connect with irssi:

irssi

/connect localhost <port> <password>

/join #42

💡 Key Learnings

Deep understanding of low-level networking (socket, bind, accept, poll)

Real-world use of STL containers (map, vector, stack)

Clean, modular OOP design

Parsing and interpreting IRC commands

Handling concurrent client connections
