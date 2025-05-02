#include <iostream>
#include <string>
#include "Server.h"
#include "Client.h"

void RunStream(int port)
{
	Server server(port);
	server.Run();
}

void RunReceiverMode(const std::string& address, int port)
{
	Client client(address, port);
	client.Run();
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 2)
		{
			int port = std::stoi(argv[1]);
			RunStream(port);
		}
		else if (argc == 3)
		{
			std::string address = argv[1];
			int port = std::stoi(argv[2]);
			RunReceiverMode(address, port);
		}
		else
		{
			std::cerr << "Usage:\n"
					  << "  Station mode: tv PORT\n"
					  << "  Receiver mode: tv ADDRESS PORT\n";
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}