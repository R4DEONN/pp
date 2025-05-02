#pragma once

#include <atomic>
#include <stdexcept>
#include <cassert>

class TicketOffice
{
public:
	explicit TicketOffice(int numTickets) : m_numTickets(numTickets)
	{
		if (numTickets < 0)
		{
			throw std::invalid_argument("Number of tickets cannot be negative");
		}
	}

	TicketOffice(const TicketOffice&) = delete;

	TicketOffice& operator=(const TicketOffice&) = delete;

	int SellTickets(int ticketsToBuy)
	{
		if (ticketsToBuy <= 0)
		{
			throw std::invalid_argument("ticketsToBuy must be positive");
		}

		int ticketsLeft = m_numTickets.load(std::memory_order_acquire);

		while (true)
		{
			if (ticketsLeft < ticketsToBuy)
			{
				return 0;
			}

			if (m_numTickets.compare_exchange_weak(ticketsLeft, ticketsLeft - ticketsToBuy,
				std::memory_order_release,
				std::memory_order_acquire))
			{
				return ticketsToBuy;
			}
		}
	}

	[[nodiscard]] int GetTicketsLeft() const noexcept
	{
		return m_numTickets.load(std::memory_order_relaxed);
	}

private:
	std::atomic<int> m_numTickets;
};
