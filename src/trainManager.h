//
// Created by Administrator on 2026/5/31.
//

#ifndef TICKETSYSTEM_TRIANMANAGER_H
#define TICKETSYSTEM_TRIANMANAGER_H

#include "../src/bpt.h"
#include "../src/vector/vector.h"
#include "../src/date.h"
#include <cstring>
#include <iostream>

// key（查询方式不同）
struct TrainIDKey {
    char trainID[21];
    TrainIDKey() {
        trainID[0] = '\0';
    }
    TrainIDKey(const char* t) {
        strncpy(trainID, t, 20);
        trainID[20] = '\0';
    }
    bool operator<(const TrainIDKey& other) const {
        return strcmp(trainID, other.trainID) < 0;
    }
    bool operator==(const TrainIDKey& other) const {
        return strcmp(trainID, other.trainID) == 0;
    }
};
// 找经过某个车站的车
struct StationKey {
    char stationName[31];
    char trainID[21];
    StationKey() {
        stationName[0] = trainID[0] = '\0';
    }
    StationKey(const char* station, const char* train) {
        strncpy(stationName, station, 30);
        stationName[30] = '\0';
        strncpy(trainID, train, 20);
        trainID[20] = '\0';
    }
    bool operator<(const StationKey& other) const {
        int temp = strcmp(stationName, other.stationName);
        if (temp != 0) {
            return temp < 0;
        }
        return strcmp(trainID, other.trainID) < 0;
    }
    bool operator==(const StationKey& other) const {
        return strcmp(stationName, other.stationName) == 0 && strcmp(trainID, other.trainID) == 0;
    }
};
// 找某趟车某天的座位
struct SeatKey {
    char trainID[21];
    int startDay;
    SeatKey() : startDay(0) {
        trainID[0] = '\0';
    }
    SeatKey(const char* t, int d) : startDay(d) {
        strncpy(trainID, t, 20);
        trainID[20] = '\0';
    }
    bool operator<(const SeatKey& other) const {
        int temp = strcmp(trainID, other.trainID);
        if (temp != 0) {
            return temp < 0;
        }
        return startDay < other.startDay;
    }
    bool operator==(const SeatKey& other) const {
        return strcmp(trainID, other.trainID) == 0 && startDay == other.startDay;
    }
};

// 对应的value
struct TrainInfo {
    char trainID[21];
    int stationNum;
    char stations[100][31];
    int seatNum;
    int prefixPrice[100]; // prefixPrice[i]: 从0到i，prefixPrice[0] = 0
    int arriveMins[100]; // arriveMins[i]: 从始发站出发后，到达第i站的累计分钟
    int departMins[100]; // departMins[i]: 从始发站出发后，离开第i站的累计分钟，就是arriveMins加上stopovertime
    int startTimeMin;
    int saleStart;
    int saleEnd;
    char type;
    bool released;
    TrainInfo() : stationNum(0), seatNum(0), startTimeMin(0), saleStart(0), saleEnd(0), type('\0'), released(false) {
        trainID[0] = '\0';
        for (int i = 0; i < 100; i++) {
            stations[i][0] = '\0';
            prefixPrice[i] = arriveMins[i] = departMins[i] = 0;
        }
    }
    bool operator<(const TrainInfo& other) const {
        return strcmp(trainID, other.trainID) < 0;
    }
    bool operator==(const TrainInfo& other) const {
        return strcmp(trainID, other.trainID) == 0;
    }
};

struct StationInfo {
    int stationIdx; // 始发站是0
    StationInfo() : stationIdx(0) {}
    StationInfo(int s) : stationIdx(s) {}
    bool operator<(const StationInfo& other) const {
        return stationIdx < other.stationIdx;
    }
    bool operator==(const StationInfo& other) const {
        return stationIdx == other.stationIdx;
    }
};

struct SeatInfo {
    int seats[99]; // seats[i]: 从i到i+1站剩余票数
    int stationNum;
    SeatInfo() : stationNum(0) {
        for (int i = 0; i < 99; i++) {
            seats[i] = 0;
        }
    }
    bool operator<(const SeatInfo& other) const {
        return false;
    }
    bool operator==(const SeatInfo& other) const {
        if (stationNum != other.stationNum) {
            return false;
        }
        for (int i = 0; i < 99; i++) {
            if (seats[i] != other.seats[i]) {
                return false;
            }
        }
        return true;
    }
};

struct TicketResult {
    char trainID[21];
    char fromStation[31];
    char toStation[31];
    int leaveAbsTime;
    int arriveAbsTime;
    int price;
    int availSeats;
};

class TrainManager {
private:
    BPlusTree<TrainIDKey, TrainInfo> trainTree;
    BPlusTree<StationKey, StationInfo> stationTree;
    BPlusTree<SeatKey, SeatInfo> seatTree;

    // 找出所有经过某个站的已发布车
    void queryByStation(const char* stationName, sjtu::vector<StationKey> &outKeys, sjtu::vector<StationInfo> &outInfos) {
        StationKey low(stationName, "");
        StationKey high;
        strncpy(high.stationName, stationName, 30);
        high.stationName[30] = '\0';
        for (int i = 0; i < 20; ++i) high.trainID[i] = '\x7f';
        high.trainID[20] = '\0';
        stationTree.findRange(low, high, outKeys, outInfos);
    }

    // 收集从 fromStation 到 toStation 在 userDepDay 的所有直达票
    void collectTickets(const char *fromStation, const char *toStation, int userDepartDay, sjtu::vector<TicketResult> &results) {
        sjtu::vector<StationKey> fromKeys, toKeys;
        sjtu::vector<StationInfo> fromInfos, toInfos;
        queryByStation(fromStation, fromKeys, fromInfos);
        queryByStation(toStation, toKeys, toInfos);

        for (int i = 0; i < fromKeys.size(); ++i) {
            const char* trainID = fromKeys[i].trainID;
            int fromIdx = fromInfos[i].stationIdx;
            int toIdx = -1;
            for (int j = 0; j < toKeys.size(); ++j) {
                if (strcmp(trainID, toKeys[j].trainID) == 0) {
                    toIdx = toInfos[j].stationIdx;
                    break;
                }
            }
            if (toIdx <= fromIdx || toIdx < 0) continue;

            TrainInfo train;
            if (!getTrainInfo(trainID, train)) continue;
            int startDay = inferStartDay(userDepartDay, train.departMins[fromIdx], train.startTimeMin);
            if (startDay < train.saleStart || startDay > train.saleEnd) continue;

            SeatInfo seat;
            if (!getSeatInfo(trainID, startDay, seat)) continue;
            int minSeats = seat.seats[fromIdx];
            for (int k = fromIdx + 1; k < toIdx; ++k) {
                if (seat.seats[k] < minSeats) minSeats = seat.seats[k];
            }

            int base = startDay * 1440 + train.startTimeMin;
            TicketResult r;
            strncpy(r.trainID, trainID, 20);
            r.trainID[20] = '\0';
            strncpy(r.fromStation, fromStation, 30);
            r.fromStation[30] = '\0';
            strncpy(r.toStation, toStation, 30);
            r.toStation[30] = '\0';
            r.leaveAbsTime = train.departMins[fromIdx] + base;
            r.arriveAbsTime = train.arriveMins[toIdx] + base;
            r.price = train.prefixPrice[toIdx] - train.prefixPrice[fromIdx];
            r.availSeats = minSeats;
            results.push_back(r);
        }
    }
    // sort ticket辅助函数
    // true: a应该排在b前面
    bool isBetter(const TicketResult &a, const TicketResult &b, bool byTime) {
        if (byTime) {
            int atime = a.arriveAbsTime -  a.leaveAbsTime;
            int btime = b.arriveAbsTime - b.leaveAbsTime;
            if (atime != btime) return atime < btime;
            return strcmp(a.trainID, b.trainID) < 0;
        } else {
            if (a.price != b.price) return a.price < b.price;
            return strcmp(a.trainID, b.trainID) < 0;
        }
    }
    void sortTickets(sjtu::vector<TicketResult> &results, bool byTime) {
        for (int i = 1; i < results.size(); ++i) {
            int j = i - 1;
            TicketResult tmp = results[i];
            while (j >= 0 && isBetter(tmp, results[j], byTime)) {
                results[j + 1] = results[j];
                --j;
            }
            results[j + 1] = tmp;
        }
    }
    void printTicket(const TicketResult &r) {
        char leave[12], arrive[12];
        absMinToStr(r.leaveAbsTime, leave);
        absMinToStr(r.arriveAbsTime, arrive);
        std::cout << r.trainID << ' ' << r.fromStation << ' ' << leave << " -> " << r.toStation
        << ' ' << arrive << ' ' << r.price << ' ' << r.availSeats << '\n';
    }

    bool isBetterTransfer(const TicketResult &ra, const TicketResult &rb,
                          const TicketResult &bestA, const TicketResult &bestB, bool byTime) {
        int totalTime  = rb.arriveAbsTime - ra.leaveAbsTime;
        int totalPrice = ra.price + rb.price;
        int bestTime   = bestB.arriveAbsTime - bestA.leaveAbsTime;
        int bestPrice  = bestA.price + bestB.price;
        if (byTime) {
            if (totalTime  != bestTime)  return totalTime  < bestTime;
            if (totalPrice != bestPrice) return totalPrice < bestPrice;
            if (strcmp(ra.trainID, bestA.trainID) != 0) return strcmp(ra.trainID, bestA.trainID) < 0;
            return strcmp(rb.trainID, bestB.trainID) < 0;
        } else {
            if (totalPrice != bestPrice) return totalPrice < bestPrice;
            if (totalTime  != bestTime)  return totalTime  < bestTime;
            if (strcmp(ra.trainID, bestA.trainID) != 0) return strcmp(ra.trainID, bestA.trainID) < 0;
            return strcmp(rb.trainID, bestB.trainID) < 0;
        }
    }
    bool findBestTransfer(const char *fromStation, const char *toStation, int userDepartDay, bool byTime,
                          TicketResult &bestA, TicketResult &bestB) {
        sjtu::vector<StationKey> aKeys;
        sjtu::vector<StationInfo> aInfos;
        queryByStation(fromStation, aKeys, aInfos);
        bool found = false;

        for (int ai = 0; ai < aKeys.size(); ++ai) {
            const char* trainIdA = aKeys[ai].trainID;
            int fromIdxA = aInfos[ai].stationIdx;
            TrainInfo trainA;
            if (!getTrainInfo(trainIdA, trainA)) continue;

            int startDayA = inferStartDay(userDepartDay, trainA.departMins[fromIdxA], trainA.startTimeMin);
            if (startDayA < trainA.saleStart || startDayA > trainA.saleEnd) continue;

            SeatInfo seatA;
            if (!getSeatInfo(trainIdA, startDayA, seatA)) continue;

            int baseA = startDayA * 1440 + trainA.startTimeMin;

            for (int midIdx = fromIdxA + 1; midIdx < trainA.stationNum; ++midIdx) {
                const char* midStation = trainA.stations[midIdx];
                if (strcmp(midStation, toStation) == 0) continue;

                int arriveAtMid = baseA + trainA.arriveMins[midIdx];
                int midArriveDay = arriveAtMid / 1440;

                for (int d = midArriveDay; d < midArriveDay + 3; ++d) {
                    sjtu::vector<TicketResult> bResults;
                    collectTickets(midStation, toStation, d, bResults);

                    for (int bi = 0; bi < bResults.size(); ++bi) {
                        TicketResult &rb = bResults[bi];
                        if (strcmp(rb.trainID, trainIdA) == 0) continue;
                        if (rb.leaveAbsTime < arriveAtMid) continue;

                        int minSeatsA = seatA.seats[fromIdxA];
                        for (int k = fromIdxA + 1; k < midIdx; ++k) if (seatA.seats[k] < minSeatsA) minSeatsA = seatA.seats[k];

                        TicketResult ra;
                        strncpy(ra.trainID, trainA.trainID, 20);
                        ra.trainID[20] = '\0';
                        strncpy(ra.fromStation, trainA.stations[fromIdxA], 30);
                        ra.fromStation[30] = '\0';
                        strncpy(ra.toStation, midStation, 30);
                        ra.toStation[30] = '\0';
                        ra.leaveAbsTime = baseA + trainA.departMins[fromIdxA];
                        ra.arriveAbsTime = baseA + trainA.arriveMins[midIdx];
                        ra.price = trainA.prefixPrice[midIdx] - trainA.prefixPrice[fromIdxA];
                        ra.availSeats = minSeatsA;

                        bool better = (!found || isBetterTransfer(ra, rb, bestA, bestB, byTime));
                        if (better) {
                            found = true;
                            bestA = ra;
                            bestB = rb;
                        }
                    }
                }
            }
        }
        return found;
    }

public:
    TrainManager() : trainTree("train"), stationTree("station"), seatTree("seat") {}

    int addTrain(const char *trainID, int stationNum, int seatNum, const char stations[][31], const int prices[],
                 const char *startTime, const int travelTimes[], const int stopoverTimes[], const char *saleDateStart,
                 const char *saleDateEnd, char type) {
        TrainIDKey key(trainID);
        sjtu::vector<TrainInfo> found;
        trainTree.findAll(key, found);
        if (!found.empty()) return -1;

        TrainInfo info;
        strncpy(info.trainID, trainID, 20);
        info.trainID[20] = '\0';
        info.stationNum = stationNum;
        info.seatNum = seatNum;
        info.startTimeMin = timeToMin(startTime);
        info.saleStart = dateToDay(saleDateStart);
        info.saleEnd = dateToDay(saleDateEnd);
        info.type = type;
        info.released = false;
        for (int i = 0; i < stationNum; i++) {
            strncpy(info.stations[i], stations[i], 30);
            info.stations[i][30] = '\0';
        }
        info.prefixPrice[0] = 0;
        info.arriveMins[0] = 0;
        info.departMins[0] = 0;
        for (int i = 1; i < stationNum; i++) {
            info.prefixPrice[i] = prices[i - 1] + info.prefixPrice[i - 1];
            info.arriveMins[i] = info.departMins[i - 1] + travelTimes[i - 1];
            info.departMins[i] = (i < stationNum - 1) ? info.arriveMins[i] + stopoverTimes[i - 1] : info.arriveMins[i];
        }
        trainTree.insert(key, info);
        return 0;
    }

    int deleteTrain(const char *trainID) {
        TrainIDKey key(trainID);
        sjtu::vector<TrainInfo> found;
        trainTree.findAll(key, found);
        if (found.empty() || found[0].released) return -1;
        trainTree.erase(key, found[0]);
        return 0;
    }

    int releaseTrain(const char *trainID) {
        TrainIDKey key(trainID);
        sjtu::vector<TrainInfo> found;
        trainTree.findAll(key, found);
        if (found.empty() || found[0].released) return -1;
        TrainInfo info = found[0];
        info.released = true;
        trainTree.erase(key, found[0]);
        trainTree.insert(key, info);

        for (int i = 0; i < info.stationNum; ++i) {
            stationTree.insert(StationKey(info.stations[i], trainID), StationInfo(i));
        }
        for (int day = info.saleStart; day <= info.saleEnd; ++day) {
            SeatInfo seat;
            seat.stationNum = info.stationNum;
            for (int i = 0; i < info.stationNum - 1; ++i) {
                seat.seats[i] = info.seatNum;
            }
            seatTree.insert(SeatKey(trainID, day), seat);
        }
        return 0;
    }

    void queryTrain(const char *trainID, const char *date) {
        int month = (date[0] - '0') * 10 + (date[1] - '0');
        if (month < 6 || month > 8) {
            std::cout << "-1\n";
            return;
        }

        TrainIDKey key(trainID);
        sjtu::vector<TrainInfo> found;
        trainTree.findAll(key, found);
        if (found.empty()) {
            std::cout << "-1\n";
            return;
        }

        TrainInfo &info = found[0];
        int queryDay = dateToDay(date);
        if (queryDay < info.saleStart || queryDay > info.saleEnd) {
            std::cout << "-1\n";
            return;
        }

        SeatInfo seat;
        if (!info.released) {
            seat.stationNum = info.stationNum;
            for (int i = 0; i < info.stationNum-1; ++i) seat.seats[i] = info.seatNum;
        } else {
            sjtu::vector<SeatInfo> seatFound;
            seatTree.findAll(SeatKey(trainID, queryDay), seatFound);
            seat = seatFound[0];
        }

        std::cout << trainID << ' ' << info.type << '\n';
        int base = queryDay * 1440 + info.startTimeMin;
        for (int i = 0; i < info.stationNum; ++i) {
            char result[12];
            std::cout << info.stations[i] << ' ';
            if (i == 0) std::cout << "xx-xx xx:xx";
            else {
                absMinToStr(base + info.arriveMins[i], result);
                std::cout << result;
            }
            std::cout << " -> ";
            if (i == info.stationNum - 1) std::cout << "xx-xx xx:xx";
            else {
                absMinToStr(base + info.departMins[i], result);
                std::cout << result;
            }
            std::cout << ' ' << info.prefixPrice[i] << ' ';
            if (i == info.stationNum - 1) std::cout << "x";
            else std::cout << seat.seats[i];
            std::cout << '\n';
        }
    }

    void queryTicket(const char* fromStation, const char* toStation, const char* date, bool byTime) {
        sjtu::vector<TicketResult> results;
        collectTickets(fromStation, toStation, dateToDay(date), results);
        sortTickets(results, byTime);
        std::cout << results.size() << '\n';
        for (int i = 0; i < results.size(); ++i) {
            printTicket(results[i]);
        }
    }

    void queryTransfer(const char* fromStation, const char* toStation, const char* date, bool byTime) {
        TicketResult bestA, bestB;
        if (!findBestTransfer(fromStation, toStation, dateToDay(date), byTime, bestA, bestB)) {
            std::cout << "0\n";
            return;
        }
        printTicket(bestA);
        printTicket(bestB);
    }

    // order manager 辅助函数
    // 找到返回true并填充，找不到返回false
    bool getTrainInfo(const char *trainID, TrainInfo &outInfo) {
        TrainIDKey key(trainID);
        sjtu::vector<TrainInfo> found;
        trainTree.findAll(key, found);
        if (found.empty()) return false;
        outInfo = found[0];
        return true;
    }
    bool getSeatInfo(const char *trainID, int startDay, SeatInfo &outInfo) {
        sjtu::vector<SeatInfo> found;
        SeatKey key(trainID, startDay);
        seatTree.findAll(key, found);
        if (found.empty()) return false;
        outInfo = found[0];
        return true;
    }
    // 修改seats[fromIdx]到seats[toIdx - 1], delta 为变化量（买票负，退票正）
    void modifySeats(const char *trainID, int startDay, int fromIdx, int toIdx, int delta) {
        SeatKey key(trainID, startDay);
        sjtu::vector<SeatInfo> found;
        seatTree.findAll(key, found);
        if (found.empty()) return;
        SeatInfo newSeat = found[0];
        for (int i = fromIdx; i < toIdx; ++i) {
            newSeat.seats[i] += delta;
        }
        seatTree.erase(key, found[0]);
        seatTree.insert(key, newSeat);
    }
};

#endif //TICKETSYSTEM_TRIANMANAGER_H