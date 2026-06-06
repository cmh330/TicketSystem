//
// Created by Administrator on 2026/5/31.
//

#ifndef TICKETSYSTEM_ORDERMANAGER_H
#define TICKETSYSTEM_ORDERMANAGER_H

#include "../src/vector/vector.h"
#include "../src/bpt.h"
#include "../src/map/map.h"
#include "../src/trainManager.h"
#include <cstring>
#include <iostream>
#include <climits>

static const int ORDER_SUCCESS = 0;
static const int ORDER_PENDING = 1;
static const int ORDER_REFUNDED = 2;

struct OrderKey {
    char username[21];
    int time; // 存负的
    OrderKey() : time(0) {
        username[0] = '\0';
    }
    OrderKey(const char *u, int t) : time(-t) {
        strncpy(username, u, 20);
        username[20] = '\0';
    }
    bool operator<(const OrderKey &other) const {
        int c = strcmp(username, other.username);
        if (c != 0) return c < 0;
        return time < other.time;
    }
    bool operator==(const OrderKey &other) const {
        return strcmp(username, other.username) == 0 && time == other.time;
    }
};

struct OrderInfo {
    char username[21];
    char trainID[21];
    char fromStation[31];
    char toStation[31];
    int  fromIdx;
    int  toIdx;
    int  startDay;
    int  leaveAbsMin;
    int  arriveAbsMin;
    int  price;
    int  num;
    int  status; // 0=success, 1=pending, 2=refunded
    int  time; // 下单时刻
    OrderInfo() : fromIdx(0), toIdx(0), startDay(0),
                  leaveAbsMin(0), arriveAbsMin(0),
                  price(0), num(0), status(0), time(0) {
        username[0] = trainID[0] = fromStation[0] = toStation[0] = '\0';
    }
    bool operator<(const OrderInfo &other) const {
        return time < other.time;
    }
    bool operator==(const OrderInfo &other) const {
        return time == other.time && strcmp(username, other.username) == 0;
    }
};

struct PendingKey {
    char trainID[21];
    int startDay;
    int time;
    PendingKey() : startDay(0), time(0) {
        trainID[0] = '\0';
    }
    PendingKey(const char* tID, int s, int t) : startDay(s), time(t) {
        strncpy(trainID, tID, 20);
        trainID[20] = '\0';
    }
    bool operator<(const PendingKey &other) const {
        int c = strcmp(trainID, other.trainID);
        if (c != 0) return c < 0;
        if (startDay != other.startDay) return startDay < other.startDay;
        return time < other.time;
    }
    bool operator==(const PendingKey &other) const {
        return strcmp(trainID, other.trainID) == 0 && startDay == other.startDay && time == other.time;
    }
};

struct PendingInfo {
    char username[21];
    int fromIdx;
    int toIdx;
    int time;
    int num; // 几张票
    PendingInfo() : fromIdx(0), toIdx(0), time(0), num(0) {
        username[0] = '\0';
    }
    bool operator<(const PendingInfo &other) const {
        return time < other.time;
    }
    bool operator==(const PendingInfo &other) const {
        return time == other.time && strcmp(username, other.username) == 0;
    }
};

class OrderManager {
private:
    BPlusTree<OrderKey, OrderInfo> orderTree;
    BPlusTree<PendingKey, PendingInfo> pendingTree;

    // queryOrder 和 refundTicket辅助
    void collectOrders(const char *username, sjtu::vector<OrderInfo> &orders) {
        OrderKey low, high;
        strncpy(low.username,  username, 20);
        low.username[20]  = '\0';
        strncpy(high.username, username, 20);
        high.username[20] = '\0';
        low.time  = INT_MIN;
        high.time = 0;
        sjtu::vector<OrderKey> keys;
        orderTree.findRange(low, high, keys, orders);
    }

    // 有人退票时处理候补
    void processQueue(const char* trainID, int startDay, TrainManager& tm) {
        PendingKey low(trainID, startDay, 0);
        PendingKey high(trainID, startDay, INT_MAX);
        sjtu::vector<PendingKey> pendingKeys;
        sjtu::vector<PendingInfo> pendingInfos;
        pendingTree.findRange(low, high, pendingKeys, pendingInfos);

        for (int i = 0; i < pendingInfos.size(); ++i) {
            PendingInfo &info = pendingInfos[i];
            SeatInfo seat;
            if (!tm.getSeatInfo(trainID, startDay, seat)) continue;
            int minSeats = seat.seats[info.fromIdx];
            for (int j = info.fromIdx + 1; j < info.toIdx; ++j) {
                if (minSeats > seat.seats[j]) minSeats = seat.seats[j];
            }
            if (minSeats < info.num) continue;

            tm.modifySeats(trainID, startDay, info.fromIdx, info.toIdx, -info.num);
            pendingTree.erase(pendingKeys[i], info);

            OrderKey ok(info.username, info.time);
            sjtu::vector<OrderInfo> orders;
            orderTree.findAll(ok, orders);
            if (!orders.empty()) {
                OrderInfo of = orders[0];
                of.status = ORDER_SUCCESS;
                orderTree.erase(ok, orders[0]);
                orderTree.insert(ok, of);
            }
        }
    }

public:
    OrderManager() : orderTree("order"), pendingTree("pending") {}

    int buyTicket(const char* username, const char* trainID, int startDay, int fromIdx, int toIdx,
                  int leaveAbsMin, int arriveAbsMin, int price, int num, bool acceptQueue, int time, TrainManager& tm) {
        SeatInfo seat;
        if (!tm.getSeatInfo(trainID, startDay, seat)) return -1;
        int minSeats = seat.seats[fromIdx];
        for (int i = fromIdx + 1; i < toIdx; i++) {
            if (seat.seats[i] < minSeats) minSeats = seat.seats[i];
        }

        TrainInfo train;
        tm.getTrainInfo(trainID, train);

        OrderInfo order;
        strncpy(order.username, username, 20);
        order.username[20] = '\0';
        strncpy(order.trainID, trainID, 20);
        order.trainID[20] = '\0';
        strncpy(order.fromStation, train.stations[fromIdx], 30);
        order.fromStation[30] = '\0';
        strncpy(order.toStation, train.stations[toIdx], 30);
        order.toStation[30] = '\0';
        order.startDay = startDay;
        order.leaveAbsMin = leaveAbsMin;
        order.arriveAbsMin = arriveAbsMin;
        order.price = price;
        order.num = num;
        order.time = time;
        order.fromIdx = fromIdx;
        order.toIdx = toIdx;

        if (minSeats >= num) {
            order.status = ORDER_SUCCESS;
            orderTree.insert(OrderKey(username, time), order);
            tm.modifySeats(trainID, startDay, fromIdx, toIdx, -num);
            return price * num;
        }

        if (!acceptQueue) return -1;

        order.status = ORDER_PENDING;
        orderTree.insert(OrderKey(username, time), order);
        PendingInfo pending;
        strncpy(pending.username, username, 20);
        pending.username[20] = '\0';
        pending.fromIdx = fromIdx;
        pending.toIdx = toIdx;
        pending.time = time;
        pending.num = num;
        pendingTree.insert(PendingKey(trainID, startDay, time), pending);
        return -2;
    }

    void queryOrder(const char* username) {
        sjtu::vector<OrderInfo> orders;
        collectOrders(username, orders);
        std::cout << orders.size() << '\n';

        for (int i = 0; i < orders.size(); ++i) {
            OrderInfo &o = orders[i];
            std::string status;
            if (o.status == ORDER_PENDING) status = "pending";
            else if (o.status == ORDER_SUCCESS) status = "success";
            else if (o.status == ORDER_REFUNDED) status = "refunded";
            char leaving[12], arriving[12];
            absMinToStr(o.leaveAbsMin, leaving);
            absMinToStr(o.arriveAbsMin, arriving);
            std::cout << '[' << status << "] " << o.trainID << ' ' << o.fromStation << ' ' << leaving
            << " -> " << o.toStation << ' ' << arriving << ' ' << o.price << ' ' << o.num << '\n';
        }
    }

    int refundTicket(const char* username, int n, TrainManager& tm) {
        sjtu::vector<OrderInfo> orders;
        collectOrders(username, orders);
        if (n < 1 || n > orders.size()) return -1;

        OrderInfo &order = orders[n - 1];
        if (order.status == ORDER_REFUNDED) return -1;

        OrderKey ok(username, order.time);
        OrderInfo newOrder = order;
        newOrder.status = ORDER_REFUNDED;
        orderTree.erase(ok, order);
        orderTree.insert(ok, newOrder);

        if (order.status == ORDER_PENDING) {
            PendingInfo pending;
            strncpy(pending.username, username, 20);
            pending.username[20] = '\0';
            pending.fromIdx = order.fromIdx;
            pending.toIdx = order.toIdx;
            pending.time = order.time;
            pending.num = order.num;
            pendingTree.erase(PendingKey(order.trainID, order.startDay, order.time), pending);
            return 0;
        }

        tm.modifySeats(order.trainID, order.startDay, order.fromIdx, order.toIdx, order.num);
        processQueue(order.trainID, order.startDay, tm);
        return 0;
    }
};


#endif //TICKETSYSTEM_ORDERMANAGER_H