#include <iostream>
#include "TicketOffice.h"

int main()
{
	TicketOffice office(10);

	std::cout << office.GetTicketsLeft() << std::endl;
	std::cout << office.SellTickets(3) << std::endl;
	std::cout << office.GetTicketsLeft() << std::endl;

	try
	{
		std::cout << office.SellTickets(-1) << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	try
	{
		std::cout << office.SellTickets(0) << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	std::cout << office.SellTickets(8) << std::endl;
//	EXPECT_EQ(office.GetTicketsLeft(), 10);
//	EXPECT_EQ(office.SellTickets(3), 3);
//	EXPECT_EQ(office.GetTicketsLeft(), 7);

//	EXPECT_THROW(office.SellTickets(-1), std::invalid_argument);
//	EXPECT_THROW(office.SellTickets(0), std::invalid_argument);

//	EXPECT_EQ(office.SellTickets(8), 0); // Нельзя продать больше, чем есть
}