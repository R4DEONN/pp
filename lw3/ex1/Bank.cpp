//
// Created by admin on 05.03.2025.
//

#include "Bank.h"

Bank::Bank(Money cash)
	: m_cash(cash)
{
	if (cash < 0)
	{
		throw BankOperationError("Initial cash cannot be negative");
	}
}

unsigned long long Bank::GetOperationsCount() const
{
	return m_operationsCount;
}

// Перевести деньги с исходного счёта (srcAccountId) на целевой (dstAccountId)
// Нельзя перевести больше, чем есть на исходном счёте
// Нельзя перевести отрицательное количество денег
// Исключение BankOperationError выбрасывается, при отсутствии счетов или
// недостатке денег на исходном счёте
// При отрицательном количестве переводимых денег выбрасывается std::out_of_range
void Bank::SendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(srcAccountId) == m_accounts.end() || m_accounts.find(dstAccountId) == m_accounts.end())
	{
		throw BankOperationError("Invalid account ID");
	}

	if (m_accounts[srcAccountId] < amount)
	{
		throw BankOperationError("Insufficient funds");
	}

	m_accounts[srcAccountId] -= amount;
	m_accounts[dstAccountId] += amount;
	m_operationsCount++;
}

// Перевести деньги с исходного счёта (srcAccountId) на целевой (dstAccountId)
// Нельзя перевести больше, чем есть на исходном счёте
// Нельзя перевести отрицательное количество денег
// При нехватке денег на исходном счёте возвращается false
// Если номера счетов невалидны, выбрасывается BankOperationError
// При отрицательном количестве денег выбрасывается std::out_of_range
bool Bank::TrySendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount)
{
	if (amount < 0) //TODO: Проверить в тесте границу (0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	//TODO: Вынести поиск в переменные
	if (m_accounts.find(srcAccountId) == m_accounts.end() || m_accounts.find(dstAccountId) == m_accounts.end())
	{
		throw BankOperationError("Invalid account ID");
	}

	if (m_accounts[srcAccountId] < amount)
	{
		return false;
	}

	m_accounts[srcAccountId] -= amount;
	m_accounts[dstAccountId] += amount;
	m_operationsCount++;

	return true;
}

Money Bank::GetCash() const
{
	//TODO: использовать shared_mutex
	return m_cash;
}

Money Bank::GetAccountBalance(AccountId accountId) const
{
	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(accountId) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	m_operationsCount++;
	return m_accounts.at(accountId);
}

void Bank::WithdrawMoney(AccountId account, Money amount)
{
	//TODO: Выразить через TryWithdraw
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(account) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	if (m_accounts[account] < amount)
	{
		throw BankOperationError("Insufficient funds");
	}

	m_accounts[account] -= amount;
	m_cash += amount;
	m_operationsCount++;
}

bool Bank::TryWithdrawMoney(AccountId account, Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(account) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	if (m_accounts[account] < amount)
	{
		return false;
	}

	m_accounts[account] -= amount;
	m_cash += amount;
	m_operationsCount++;

	return true;
}

void Bank::DepositMoney(AccountId account, Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(account) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	if (m_cash < amount)
	{
		throw BankOperationError("Insufficient cash in the bank");
	}

	m_accounts[account] += amount;
	m_cash -= amount;
	m_operationsCount++;
}

bool Bank::TryDepositMoney(AccountId account, Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(account) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	if (m_cash < amount)
	{
		return false;
	}

	m_accounts[account] += amount;
	m_cash -= amount;
	m_operationsCount++;

	return true;
}

AccountId Bank::OpenAccount()
{
	std::lock_guard<std::mutex> lock(m_mtx);

	AccountId accountId = m_nextAccountId++;
	m_accounts[accountId] = 0;

	return accountId;
}

Money Bank::CloseAccount(AccountId accountId)
{
	std::lock_guard<std::mutex> lock(m_mtx);

	if (m_accounts.find(accountId) == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	auto balance = m_accounts[accountId];
	m_cash += balance;
	m_accounts.erase(accountId);
	m_operationsCount++;

	return balance;
}

