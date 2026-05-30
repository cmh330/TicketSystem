//
// Created by Administrator on 2026/5/30.
//

#ifndef TICKETSYSTEM_DATE_H
#define TICKETSYSTEM_DATE_H

// 日期：用距离 6.1 的天数表示，6.1 = 0
// 时间：用当天分钟数表示，00：00 = 0
// 绝对时间：从 6.1，00：00 开始算起的分钟数

inline int daysInMonth(int month) {
    if (month == 7 || month == 8) return 31;
    return 30;
}

// "mm-dd" -> 距离 6.1 的天数
inline int dateToDay(const char* s) {
    int month = (s[0] - '0') * 10 + s[1] - '0';
    int day = (s[3] - '0') * 10 + s[4] - '0';
    int d = 0;
    for (int m = 6; m < month; ++m) d += daysInMonth(m);
    d += day - 1;
    return d;
}

// 距离 6.1 的天数 -> "mm-dd"
inline void dayToDate(int d, char* result) {
    int month = 6;
    while (d >= daysInMonth(month)) {
        d -= daysInMonth(month);
        ++month;
    }
    int day = d + 1;
    result[0] = static_cast<char>('0' + month / 10);
    result[1] = static_cast<char>('0' + month % 10);
    result[2] = '-';
    result[3] = static_cast<char>('0' + day / 10);
    result[4] = static_cast<char>('0' + day % 10);
    result[5] = '\0';
}

// "hh:mm" -> 分钟数
inline int timeToMin(const char* s) {
    return ((s[0] - '0') * 10 + s[1] - '0') * 60 + (s[3] - '0') * 10 + s[4] - '0';
}

// 分钟数 -> "hh:mm"
inline void minToTime(int min, char* result) {
    min = ((min % 1440) + 1440) % 1440;
    result[0] = static_cast<char>((min / 60) / 10 + '0');
    result[1] = static_cast<char>((min / 60) % 10 + '0');
    result[2] = ':';
    result[3] = static_cast<char>((min % 60) / 10 + '0');
    result[4] = static_cast<char>((min % 60) % 10 + '0');
    result[5] = '\0';
}

// 绝对时间 -> "mm-dd hh:mm"
inline void absMinToStr(int absMin, char* result) {
    dayToDate(absMin / 1440, result);
    result[5] = ' ';
    minToTime(absMin % 1440, result + 6);
    result[11] = '\0';
}

// 已知用户给的从某站出发的日期 userDepartureDay（dateToDay 结果）
// minFromStart：从始发站出发到该站离站的分钟
// startTimeMin：始发站发车时刻（分钟）
// 从用户给的时间推算出发日期
inline int inferStartDay(int userDepartureDay, int minFromStart, int startTimeMin) {
    return userDepartureDay - (minFromStart + startTimeMin) / 1440;
}

#endif //TICKETSYSTEM_DATE_H