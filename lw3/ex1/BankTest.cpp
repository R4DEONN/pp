#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include "Bank.h"

TEST_CASE("Bank initialization", "[Bank]")
{
	SECTION("Valid initial cash")
	{
		Bank bank(1000);
		REQUIRE(bank.GetCash() == 1000);
	}

	SECTION("Negative initial cash throws exception")
	{
		REQUIRE_THROWS_AS(Bank(-100), BankOperationError);
	}
}

TEST_CASE("Account management", "[Bank]")
{
	Bank bank(1000);

	SECTION("Open account")
	{
		AccountId acc1 = bank.OpenAccount();
		REQUIRE(bank.GetAccountBalance(acc1) == 0);
	}

	SECTION("Close account")
	{
		AccountId acc1 = bank.OpenAccount();
		bank.DepositMoney(acc1, 500);
		Money balance = bank.CloseAccount(acc1);
		REQUIRE(balance == 500);
		REQUIRE(bank.GetCash() == 1000);
		REQUIRE_THROWS_AS(bank.GetAccountBalance(acc1), BankOperationError);
	}
}

TEST_CASE("Deposit and withdraw money", "[Bank]")
{
	Bank bank(1000);
	AccountId acc1 = bank.OpenAccount();

	SECTION("Deposit money")
	{
		bank.DepositMoney(acc1, 300);
		REQUIRE(bank.GetAccountBalance(acc1) == 300);
		REQUIRE(bank.GetCash() == 700);
	}

	SECTION("Deposit negative amount throws exception")
	{
		REQUIRE_THROWS_AS(bank.DepositMoney(acc1, -100), std::out_of_range);
	}

	SECTION("Withdraw money")
	{
		bank.DepositMoney(acc1, 500);
		bank.WithdrawMoney(acc1, 200);
		REQUIRE(bank.GetAccountBalance(acc1) == 300);
		REQUIRE(bank.GetCash() == 700);
	}

	SECTION("Withdraw negative amount throws exception")
	{
		REQUIRE_THROWS_AS(bank.WithdrawMoney(acc1, -100), std::out_of_range);
	}

	SECTION("Withdraw more than balance throws exception")
	{
		bank.DepositMoney(acc1, 100);
		REQUIRE_THROWS_AS(bank.WithdrawMoney(acc1, 200), BankOperationError);
	}

	SECTION("TryWithdrawMoney returns false on insufficient funds")
	{
		bank.DepositMoney(acc1, 100);
		REQUIRE_FALSE(bank.TryWithdrawMoney(acc1, 200));
		REQUIRE(bank.GetAccountBalance(acc1) == 100);
	}
}

TEST_CASE("Money transfer", "[Bank]")
{
	Bank bank(1000);
	AccountId acc1 = bank.OpenAccount();
	AccountId acc2 = bank.OpenAccount();
	bank.DepositMoney(acc1, 500);

	SECTION("Send money")
	{
		bank.SendMoney(acc1, acc2, 200);
		REQUIRE(bank.GetAccountBalance(acc1) == 300);
		REQUIRE(bank.GetAccountBalance(acc2) == 200);
	}

	SECTION("Send money with insufficient funds throws exception")
	{
		REQUIRE_THROWS_AS(bank.SendMoney(acc1, acc2, 600), BankOperationError);
	}

	SECTION("TrySendMoney returns false on insufficient funds")
	{
		REQUIRE_FALSE(bank.TrySendMoney(acc1, acc2, 600));
		REQUIRE(bank.GetAccountBalance(acc1) == 500);
	}

	SECTION("Send money to non-existent account throws exception")
	{
		REQUIRE_THROWS_AS(bank.SendMoney(acc1, 999, 100), BankOperationError);
	}

	SECTION("Send negative amount throws exception")
	{
		REQUIRE_THROWS_AS(bank.SendMoney(acc1, acc2, -100), std::out_of_range);
	}
}

TEST_CASE("Operations count", "[Bank]")
{
	Bank bank(1000);
	AccountId acc1 = bank.OpenAccount();
	AccountId acc2 = bank.OpenAccount();

	SECTION("Initial operations count is zero")
	{
		REQUIRE(bank.GetOperationsCount() == 0);
	}

	SECTION("Operations count increases with each operation")
	{
		bank.DepositMoney(acc1, 100);
		bank.WithdrawMoney(acc1, 50);
		bank.SendMoney(acc1, acc2, 25);
		REQUIRE(bank.GetOperationsCount() == 3);
	}
}