#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <utility>
#include <vector>
#include <csignal>
#include <syncstream>
#include "Bank.h"

using namespace std::chrono_literals;

class Person
{
protected:
	Bank& bank;
	AccountId accountId;
	std::string name;
	std::atomic<bool>& running;

public:
	Person(Bank& bank, std::string name, std::atomic<bool>& running)
		: bank(bank), name(std::move(name)), running(running)
	{
		accountId = bank.OpenAccount();
	}

	AccountId GetAccountId()
	{
		return accountId;
	}

	virtual void act() = 0; // Действия персонажа
	void ActAsync()
	{
		while (running)
		{
			try
			{
				act();
			}
			catch (const std::exception& e)
			{
				log(std::string("Error: ") + e.what());
			}
		}
	}

	virtual ~Person() = default;

	void log(const std::string& message)
	{
		std::osyncstream(std::cout) << name << ": " << message << std::endl;
	}
};

class Homer : public Person
{
private:
	AccountId margeAccountId;
	AccountId electricCompanyAccountId;
	AccountId bartAndLisaId;

public:
	Homer(Bank& bank, AccountId margeAccountId, AccountId electricCompanyAccountId, AccountId bartAndLisaId, std::atomic<bool>& running)
		: Person(bank, "Homer", running), margeAccountId(margeAccountId),
		  electricCompanyAccountId(electricCompanyAccountId), bartAndLisaId(bartAndLisaId)
	{
	}

	void act() override
	{
		if (running)
		{
			if (bank.TrySendMoney(accountId, margeAccountId, 500))
			{
				log("Transferred 500 to Marge");
			}
			else
			{
				log("Failed to transfer money to Marge");
			}

			if (bank.TrySendMoney(accountId, electricCompanyAccountId, 400))
			{
				log("Paid 400 for electricity");
			}
			else
			{
				log("Failed to pay for electricity");
			}

			if (bank.TrySendMoney(accountId, bartAndLisaId, 100))
			{
				log("Withdrew 100 for Bart and Lisa");
			}
			else
			{
				log("Failed to withdraw money for Bart and Lisa");
			}

			std::this_thread::sleep_for(200ms);
		}
	}
};

class Marge : public Person
{
private:
	AccountId apuAccountId;

public:
	Marge(Bank& bank, AccountId apuAccountId, std::atomic<bool>& running)
		: Person(bank, "Marge", running), apuAccountId(apuAccountId)
	{
	}

	void act() override
	{
		if (running)
		{
			if (bank.TrySendMoney(accountId, apuAccountId, 500))
			{
				log("Paid 500 to Apu for groceries");
			}
			else
			{
				log("Failed to pay Apu for groceries");
			}

			std::this_thread::sleep_for(300ms);
		}
	}
};

class BartLisa : public Person
{
private:
	AccountId apuAccountId;

public:
	BartLisa(Bank& bank, AccountId apuAccountId, std::atomic<bool>& running)
		: Person(bank, "Bart & Lisa", running), apuAccountId(apuAccountId)
	{
	}

	void act() override
	{
		if (running)
		{
			// Барт и Лиза тратят наличные
			if (bank.TryWithdrawMoney(accountId, 100))
			{
				log("Spent 100 at Apu's store");
			}
			else
			{
				log("Failed to spend money at Apu's store");
			}

			std::this_thread::sleep_for(400ms);
		}
	}
};

class Apu : public Person
{
private:
	AccountId electricCompanyAccountId;

public:
	Apu(Bank& bank, AccountId electricCompanyAccountId, std::atomic<bool>& running)
		: Person(bank, "Apu", running), electricCompanyAccountId(electricCompanyAccountId)
	{
	}

	void act() override
	{
		if (running)
		{
			// Апу платит за электричество
			if (bank.TrySendMoney(accountId, electricCompanyAccountId, 400))
			{
				log("Paid 400 for electricity");
			}
			else
			{
				log("Failed to pay for electricity");
			}

			// Апу вносит наличные на счёт
			if (bank.TryDepositMoney(accountId, 200))
			{
				log("Deposited 200 to bank account");
			}
			else
			{
				log("Failed to deposit money to bank account");
			}

			std::this_thread::sleep_for(500ms);
		}
	}
};

class Burns : public Person
{
private:
	AccountId homerAccountId;

public:
	Burns(Bank& bank, AccountId homerAccountId, std::atomic<bool>& running)
		: Person(bank, "Mr. Burns", running), homerAccountId(homerAccountId)
	{
	}

	void act() override
	{
		if (running)
		{
			if (bank.TryDepositMoney(accountId, 1000))
			{
				log("Get 1000");
			}
			else
			{
				log("Failed to get 1000");
			}

			// Мистер Бернс платит зарплату Гомеру
			if (bank.TrySendMoney(accountId, homerAccountId, 1000))
			{
				log("Paid 1000 salary to Homer");
			}
			else
			{
				log("Failed to pay salary to Homer");
			}

			std::this_thread::sleep_for(500ms);
		}
	}
};

std::atomic<bool> running(true);

void signalHandler(int signal)
{
	running = false;
}

int main(int argc, char* argv[])
{

	try
	{
		if (argc < 2)
		{
			std::cerr << "Usage: " << argv[0] << " <duration> <mode>" << std::endl;
			return 1;
		}

		int duration = std::stoi(argv[1]);
		std::string mode = argv[2];

		Bank bank(10000);

		AccountId electricCompanyAccountId = bank.OpenAccount();

		Apu apu(bank, electricCompanyAccountId, running);
		Marge marge(bank, apu.GetAccountId(), running);
		Homer homer(bank, marge.GetAccountId(), electricCompanyAccountId, running);
		BartLisa bartLisa(bank, apu.GetAccountId(), running);
		Burns burns(bank, homer.GetAccountId(), running);

		// Обработка сигналов
		std::signal(SIGINT, signalHandler);
		std::signal(SIGTERM, signalHandler);

		// Запуск симуляции
		if (mode == "single")
		{
			// Однопоточный режим
			auto start = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - start < std::chrono::seconds(duration) && running)
			{
				homer.act();
				marge.act();
				bartLisa.act();
				apu.act();
				burns.act();
			}
		}
		else if (mode == "multi")
		{
			std::vector<std::thread> threads;
			threads.emplace_back(&Homer::ActAsync, &homer);
			threads.emplace_back(&Marge::ActAsync, &marge);
			threads.emplace_back(&BartLisa::ActAsync, &bartLisa);
			threads.emplace_back(&Apu::ActAsync, &apu);
			threads.emplace_back(&Burns::ActAsync, &burns);

			auto start = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - start < std::chrono::seconds(duration) && running)
			{}
			running = false;

			for (auto& thread: threads)
			{
				thread.join();
			}
		}
		else
		{
			std::cerr << "Invalid mode. Use 'single' or 'multi'." << std::endl;
			return 1;
		}

		std::cout << "Total operations: " << bank.GetOperationsCount() << std::endl;
		std::cout << "Total cash in bank: " << bank.GetCash() << std::endl;

	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}