//
// Created by Administrator on 2026/6/9.
//

#ifndef TICKETSYSTEM_STORAGE_H
#define TICKETSYSTEM_STORAGE_H

#include <fstream>
#include <cstring>

template <class T>
class Storage {
private:
    std::fstream file;
    std::string filename;

public:
    Storage(const std::string &name) : filename(name) {
        file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) {
            file.open(filename, std::ios::binary | std::ios::out);
            file.close();
            file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
        }
    }

    ~Storage() {
        file.close();
    }

    int write(const T &t) {
        file.seekp(0, std::ios::end);
        int offset = file.tellp();
        file.write(reinterpret_cast<const char*>(&t), sizeof(T));
        return offset;
    }
    void read(int offset, T &t) {
        file.seekg(offset);
        file.read(reinterpret_cast<char*>(&t), sizeof(T));
    }
    void update(int offset, const T &t) {
        file.seekp(offset);
        file.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }
};

#endif //TICKETSYSTEM_STORAGE_H