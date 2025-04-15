#ifndef WAREHOUSE_THREADS_H
#define WAREHOUSE_THREADS_H

#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <syncstream>
#include "Warehouse.h"

void Supplier(Warehouse& warehouse, std::atomic<int>& totalSupplied, std::atomic<bool>& stop)
{
    while (!stop.load())
    {
        int amount = rand() % 10 + 1;
        if (warehouse.AddGoods(amount))
        {
            totalSupplied += amount;
            //std::osyncstream(std::cout) << ">>\n" << "Supplier added " << amount
                                 //       << " goods. Total supplied: " << totalSupplied << "\n";
        }
    }
}

void Client(Warehouse& warehouse, std::atomic<int>& totalPurchased, std::atomic<bool>& stop)
{
    while (!stop.load())
    {
        int amount = rand() % 10 + 1;
        if (warehouse.TakeGoods(amount))
        {
            totalPurchased += amount;
            //std::osyncstream(std::cout) << ">>\n" << "Client purchased " << amount
            //                           << " goods. Total purchased: " << totalPurchased << "\n";
        }
    }
}

void Auditor(Warehouse& warehouse, std::atomic<bool>& stop)
{
    while (!stop.load())
    {
        int stock = warehouse.GetStock();
        //std::osyncstream(std::cout) << ">>\n" << "Auditor: Current stock is " << stock << "\n";
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

#endif // WAREHOUSE_THREADS_H