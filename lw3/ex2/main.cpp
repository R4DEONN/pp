#include "src/Threads.h"
#include "src/Warehouse.h"
#include <iostream>
#include <vector>
#include <thread>
#include <csignal>
#include <atomic>

struct Args
{
    //начинаем с маленькой буквы
    int NumSuppliers;
    int NumClients;
    int NumAuditors;
};

Args ParseArgs(int argc, char* argv[])
{
    if (argc != 4)
    {
        throw std::invalid_argument(
                "Usage: " + std::string(argv[0]) + " NUM_SUPPLIERS NUM_CLIENTS NUM_AUDITORS\n"
        );
    }

    Args args{};

    try
    {
        args.NumSuppliers = std::stoi(argv[1]);
        args.NumClients = std::stoi(argv[2]);
        args.NumAuditors = std::stoi(argv[3]);
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument("Arguments must be valid numbers.\n");
    }
    catch (const std::out_of_range&)
    {
        throw std::invalid_argument("Arguments are out of range.\n");
    }

    return args;
}

std::atomic<bool> stop(false);

void SignalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        stop.store(true);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        Args args = ParseArgs(argc, argv);

        Warehouse warehouse(100, stop);
        std::atomic<int> totalSupplied(0);
        std::atomic<int> totalPurchased(0);

		auto notifyHandler = [&warehouse](int signal) {
			if (signal == SIGINT || signal == SIGTERM)
			{
				stop.store(true);
				warehouse.NotifyAll();
			}
		};

        std::signal(SIGINT, notifyHandler);
        std::signal(SIGTERM, notifyHandler);

        {
            std::vector<std::jthread> suppliers;
            std::vector<std::jthread> clients;
            std::vector<std::jthread> auditors;

            suppliers.reserve(args.NumSuppliers);
            {
                for (int i = 0; i < args.NumSuppliers; ++i)
                {
                    suppliers.emplace_back(Supplier, std::ref(warehouse), std::ref(totalSupplied), std::ref(stop));
                }

                clients.reserve(args.NumClients);
                for (int i = 0; i < args.NumClients; ++i)
                {
                    clients.emplace_back(Client, std::ref(warehouse), std::ref(totalPurchased), std::ref(stop));
                }

                auditors.reserve(args.NumAuditors);
                for (int i = 0; i < args.NumAuditors; ++i)
                {
                    auditors.emplace_back(Auditor, std::ref(warehouse), std::ref(stop));
                }
            }
        }

        std::cout << "Total supplied: " << totalSupplied << "\n";
        std::cout << "Total purchased: " << totalPurchased << "\n";
        std::cout << "Remaining stock: " << warehouse.GetStock() << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}