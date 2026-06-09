#include <iostream>
#include <cstring>
#include <cstdio>
#include "../src/date.h"
#include "../src/orderManager.h"
#include "../src/trainManager.h"
#include "../src/userManager.h"

struct Args {
    char values[128][4000]; // values['a']对应a参数的值
    bool hasValue[128];
    Args() {
        for (int i = 0; i < 128; i++) {
            hasValue[i] = false;
            values[i][0] = '\0';
        }
    }
    void set(char key, const char *val) {
        hasValue[static_cast<int>(key)] = true;
        strncpy(values[static_cast<int>(key)], val, 3999);
        values[static_cast<int>(key)][3999] = '\0';
    }
    const char *get(char key) const {
        return hasValue[static_cast<int>(key)] ? values[static_cast<int>(key)] : nullptr;
    }
    int getInt(char key, int Default = 0) const {
        if (!hasValue[static_cast<int>(key)]) return Default;
        const char *s = values[static_cast<int>(key)];
        int x = 0;
        for (int i = 0; s[i] != '\0'; ++i) {
            x = x * 10 + s[i] - '0';
        }
        return x;
    }
};

void parseLine(const char* line, int &time, char* command, Args &args) {
    int i = 0;
    while (line[i] != '\0' && line[i] != '[') ++i;
    ++i;

    time = 0;
    while (line[i] != '\0' && line[i] != ']') {
        time = time * 10 + line[i++] - '0';
    }
    ++i;

    while (line[i] == ' ') ++i;
    int ci = 0;
    while (line[i] != '\0' && line[i] != ' ') command[ci++] = line[i++];
    command[ci] = '\0';

    while (line[i] != '\0') {
        while (line[i] == ' ') ++i;
        if (line[i] == '-') {
            char key = line[i + 1];
            i += 2;
            while (line[i] == ' ') ++i;
            char value[4000];
            int vi = 0;
            while (line[i] != '\0' && line[i] != ' ') value[vi++] = line[i++];
            value[vi] = '\0';
            args.set(key, value);
        } else {
            ++i;
        }
    }
}

// addTrain中拆分"a|b|c", 返回拆出了几个
int split(const char* s, char out[][31], int maxn) {
    int cnt = 0, i = 0, j = 0;
    while (s[j] != '\0' && cnt < maxn) {
        if (s[j] == '|') {
            out[cnt++][i] = '\0';
            i = 0;
        } else {
            out[cnt][i++] = s[j];
        }
        ++j;
    }
    out[cnt][i] = '\0';
    return cnt + 1;
}
// addTrain中拆分"1|2|3", 返回拆出了几个
int splitInt(const char* s, int out[], int maxn) {
    int cnt = 0, cur = 0, j = 0;
    while (s[j] != '\0' && cnt < maxn) {
        if (s[j] == '|') {
            out[cnt++] = cur;
            cur = 0;
        } else {
            cur = cur * 10 + s[j] - '0';
        }
        ++j;
    }
    out[cnt] = cur;
    return cnt + 1;
}

int main() {
    // for (const char *name : {"user", "user.idx", "train", "train.idx", "train.data"
    //                          "station", "station.idx", "seat", "seat.idx", "seat.data"
    //                          "order", "order.idx", "order.data", "pending", "pending.idx", "pending.data", "user_count"}) {
    //     remove(name);
    // }

    UserManager um;
    OrderManager om;
    TrainManager tm;

    char line[4000];
    while (std::cin.getline(line, sizeof(line))) {
        if (line[0] == '\0') continue;
        int time = 0;
        char command[20] = {};
        Args args;
        parseLine(line, time, command, args);
        std::cout << '[' << time << "] ";

        if (strcmp(command, "exit") == 0) {
            um.logoutAll();
            std::cout << "bye\n";
            break;
        }

        if (strcmp(command, "clean") == 0) {
            um.resetMemory();
            for (const char *name : {"user", "user.idx", "train", "train.idx",
                                     "station", "station.idx", "seat", "seat.idx",
                                     "order", "order.idx", "pending", "pending.idx", "user_count"}) {
                remove(name);
            }
            std::cout << "0\n";
        }

        else if (strcmp(command, "add_user") == 0) {
            std::cout << um.addUser(args.get('c'), args.get('u'), args.get('p'),
                              args.get('n'), args.get('m'), args.getInt('g')) << '\n';
        }

        else if (strcmp(command, "login") == 0) {
            std::cout << um.login(args.get('u'), args.get('p')) << '\n';
        }

        else if (strcmp(command, "logout") == 0) {
            std::cout << um.logout(args.get('u')) << '\n';
        }

        else if (strcmp(command, "query_profile") == 0) {
            UserInfo info;
            if (um.queryProfile(args.get('c'), args.get('u'), info) == 0) {
                std::cout << info.username << ' ' << info.name << ' ' << info.mailAddr << ' ' << info.privilege << '\n';
            } else std::cout << "-1\n";
        }

        else if (strcmp(command, "modify_profile") == 0) {
            int g = args.hasValue[static_cast<int>('g')] ? args.getInt('g') : -1;
            UserInfo info;
            if (um.modifyProfile(args.get('c'), args.get('u'), args.get('p'), args.get('n'),
                      args.get('m'), g, info) == 0) {
                std::cout << info.username << ' ' << info.name << ' ' << info.mailAddr << ' ' << info.privilege << '\n';
            } else std::cout << "-1\n";
        }

        else if (strcmp(command, "add_train") == 0) {
            char stations[100][31];
            int prices[99], travelT[99], stopoverT[99] = {};
            split(args.get('s'), stations, 100);
            splitInt(args.get('p'), prices, 99);
            splitInt(args.get('t'), travelT, 99);
            const char *o = args.get('o');
            if (o[0] != '_') splitInt(o, stopoverT, 98);

            // -d 是 "mm-dd|mm-dd"，要拆成两个
            char saleStart[6], saleEnd[6];
            const char *d = args.get('d');
            int di = 0;
            while (d[di] != '|') {
                saleStart[di] = d[di];
                ++di;
            }
            saleStart[di] = '\0';
            int ei = 0;
            ++di;
            while (d[di] != '\0') {
                saleEnd[ei++] = d[di++];
            }
            saleEnd[ei] = '\0';

            std::cout << tm.addTrain(args.get('i'), args.getInt('n'), args.getInt('m'),
                                     stations, prices, args.get('x'), travelT, stopoverT,
                                     saleStart, saleEnd, args.get('y')[0]) << '\n';
        }

        else if (strcmp(command, "delete_train") == 0) {
            std::cout << tm.deleteTrain(args.get('i')) << '\n';
        }

        else if (strcmp(command, "release_train") == 0) {
            std::cout << tm.releaseTrain(args.get('i')) << '\n';
        }

        else if (strcmp(command, "query_train") == 0) {
            tm.queryTrain(args.get('i'), args.get('d'));
        }

        else if (strcmp(command, "query_ticket") == 0) {
            const char *p = args.get('p');
            bool byTime = !(p && strcmp(p, "cost") == 0);
            tm.queryTicket(args.get('s'), args.get('t'), args.get('d'), byTime);
        }

        else if (strcmp(command, "query_transfer") == 0) {
            const char *p = args.get('p');
            bool byTime = !(p && strcmp(p, "cost") == 0);
            tm.queryTransfer(args.get('s'), args.get('t'), args.get('d'), byTime);
        }

        else if (strcmp(command, "buy_ticket") == 0) {
            const char *u = args.get('u');
            if (!um.isLoggedIn(u)) {
                std::cout << "-1\n";
                continue;
            }

            const char *tid = args.get('i');
            TrainInfo train;
            if (!tm.getTrainInfo(tid, train) || !train.released) {
                std::cout << "-1\n";
                continue;
            }

            int fromIdx = -1, toIdx = -1;
            const char *f = args.get('f'), *t = args.get('t');
            for (int k = 0; k < train.stationNum; ++k) {
                if (strcmp(train.stations[k], f) == 0) fromIdx = k;
                if (strcmp(train.stations[k], t) == 0) toIdx = k;
            }
            if (fromIdx < 0 || toIdx < 0 || fromIdx >= toIdx) {
                std::cout << "-1\n";
                continue;
            }

            int userDepartDay = dateToDay(args.get('d'));
            int startDay = inferStartDay(userDepartDay, train.departMins[fromIdx], train.startTimeMin);
            if (startDay < train.saleStart || startDay > train.saleEnd) {
                std::cout << "-1\n";
                continue;
            }

            int base = startDay * 1440 + train.startTimeMin;
            int leaveAbsMin = base + train.departMins[fromIdx];
            int arriveAbsMin = base + train.arriveMins[toIdx];
            int price = train.prefixPrice[toIdx] - train.prefixPrice[fromIdx];
            const char *q = args.get('q');
            bool acceptQueue = (q && strcmp(q, "true") == 0);

            int tmp = om.buyTicket(u, tid, startDay, fromIdx, toIdx,
                                   leaveAbsMin, arriveAbsMin, price, args.getInt('n'), acceptQueue, time, tm);
            if (tmp == -2) std::cout << "queue\n";
            else std::cout << tmp << '\n';
        }

        else if (strcmp(command, "query_order") == 0) {
            const char *u = args.get('u');
            if (!um.isLoggedIn(u)) {
                std::cout << "-1\n";
                continue;
            }
            om.queryOrder(u);
        }

        else if (strcmp(command, "refund_ticket") == 0) {
            const char *u = args.get('u');
            if (!um.isLoggedIn(u)) {
                std::cout << "-1\n";
                continue;
            }
            int n = args.getInt('n', 1);
            std::cout << om.refundTicket(u, n, tm) << '\n';
        }
    }

    return 0;
}