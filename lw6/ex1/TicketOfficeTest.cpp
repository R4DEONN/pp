#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>
#include <numeric>
#include "TicketOffice.h"
#include "thread"
#include "vector"

TEST_CASE("Single-threaded tests", "[TicketOffice]")
{
	TicketOffice office(10);

	SECTION("Initial state")
	{
		REQUIRE(office.GetTicketsLeft() == 10);
	}

	SECTION("Sell tickets successfully")
	{
		REQUIRE(office.SellTickets(3) == 3);
		REQUIRE(office.GetTicketsLeft() == 7);
	}

	SECTION("Invalid arguments")
	{
		REQUIRE_THROWS_AS(office.SellTickets(-1), std::invalid_argument);
		REQUIRE_THROWS_AS(office.SellTickets(0), std::invalid_argument);
	}

	SECTION("Attempt to sell more tickets than available")
	{
		REQUIRE(office.SellTickets(15) == 0);
		REQUIRE(office.GetTicketsLeft() == 10);
	}
}

TEST_CASE("Multi-threaded tests", "[TicketOffice]")
{
	TicketOffice office(100);
	const int numThreads = 10;
	std::vector<int> results(numThreads, 0);

	{
		const int ticketsPerThread = 10;
		std::vector<std::jthread> threads;

		for (int i = 0; i < numThreads; ++i)
		{
			threads.emplace_back([&, i]() {
				results[i] = office.SellTickets(ticketsPerThread);
			});
		}
	}

	SECTION("Total tickets sold does not exceed available")
	{
		int totalSold = std::accumulate(results.begin(), results.end(), 0);
		REQUIRE(totalSold == 100);
	}

	SECTION("No tickets left after selling")
	{
		REQUIRE(office.GetTicketsLeft() == 0);
	}
}

TEST_CASE("Over-sell attempt in multi-threaded environment", "[TicketOffice]")
{
	TicketOffice office(50);
	const int numThreads = 10;
	std::vector<int> results(numThreads, 0);

	{
		const int ticketsPerThread = 10;
		std::vector<std::jthread> threads;

		for (int i = 0; i < numThreads; ++i)
		{
			threads.emplace_back([&, i]() {
				results[i] = office.SellTickets(ticketsPerThread);
			});
		}
	}

	SECTION("Total tickets sold does not exceed available")
	{
		int totalSold = std::accumulate(results.begin(), results.end(), 0);
		REQUIRE(totalSold <= 50);
	}

	SECTION("No tickets left after selling")
	{
		REQUIRE(office.GetTicketsLeft() == 0);
	}
}