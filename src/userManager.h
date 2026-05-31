//
// Created by Administrator on 2026/5/31.
//

#ifndef TICKETSYSTEM_USERMANAGER_H
#define TICKETSYSTEM_USERMANAGER_H

#include "../src/map/map.h"
#include "../src/vector/vector.h"
#include "../src/bpt.h"
#include <cstring>

// 作为bpt的key
struct Username {
    char username[21];
    Username() {
        username[0] = '\0';
    }
    Username(const char* u) {
        strncpy(username, u, 20);
        username[20] = '\0';
    }
    bool operator<(const Username& other) const {
        return strcmp(username, other.username) < 0;
    }
    bool operator==(const Username& other) const {
        return strcmp(username, other.username) == 0;
    }
};

// bpt value
struct UserInfo {
    char username[21];
    char password[31];
    char name[51];
    char mailAddr[31];
    int privilege;
    UserInfo() : privilege(0) {
        username[0] = password[0] = name[0] = mailAddr[0] = '\0';
    }
    bool operator<(const UserInfo& other) const {
        return strcmp(username, other.username) < 0;
    }
    bool operator==(const UserInfo& other) const {
        return strcmp(username, other.username) == 0;
    }
};

class UserManager {
private:
    BPlusTree<Username, UserInfo> userTree;
    sjtu::map<Username, int> loggedIn; // int是privilege
    int userCount = 0; // 主要为了记录是不是首次创建

    void loadUserCount() {
        std::fstream f("user_count", std::fstream::in | std::fstream::binary);
        if (!f.is_open()) {
            userCount = 0;
            return;
        }
        f.read(reinterpret_cast<char*>(&userCount), sizeof(userCount));
    }
    void saveUserCount() {
        std::fstream f("user_count", std::fstream::out | std::fstream::binary);
        if (f.is_open()) {
            f.write(reinterpret_cast<char*>(&userCount), sizeof(userCount));
        }
    }

public:
    UserManager() : userTree("user") {
        loadUserCount();
    }

    // 0成功，1失败
    int addUser(const char *cur_username, const char *username, const char *password, const char *name,
                const char *mailAddr, int privilege) {
        Username newKey(username);
        if (userCount == 0) {
            UserInfo newUser;
            strncpy(newUser.username, username, 20);
            newUser.username[20] = '\0';
            strncpy(newUser.password, password, 30);
            newUser.password[30] = '\0';
            strncpy(newUser.name, name, 50);
            newUser.name[50] = '\0';
            strncpy(newUser.mailAddr, mailAddr, 30);
            newUser.mailAddr[30] = '\0';
            newUser.privilege = 10;
            userTree.insert(newKey, newUser);
            ++userCount;
            saveUserCount();
            return 0;
        }

        // 检查cur是否已登录
        Username curKey(cur_username);
        auto it = loggedIn.find(curKey);
        if (it == loggedIn.end()) return -1;
        if (privilege >= it->second) return -1;

        // username已存在
        sjtu::vector<UserInfo> found;
        userTree.findAll(newKey, found);
        if (!found.empty()) return -1;

        UserInfo newUser;
        strncpy(newUser.username, username, 20);
        newUser.username[20] = '\0';
        strncpy(newUser.password, password, 30);
        newUser.password[30] = '\0';
        strncpy(newUser.name, name, 50);
        newUser.name[50] = '\0';
        strncpy(newUser.mailAddr, mailAddr, 30);
        newUser.mailAddr[30] = '\0';
        newUser.privilege = privilege;
        userTree.insert(newKey, newUser);
        ++userCount;
        saveUserCount();
        return 0;
    }

    int login(const char *username, const char *password) {
        Username key(username);
        // 已经登陆
        auto it = loggedIn.find(key);
        if (it != loggedIn.end()) return -1;

        sjtu::vector<UserInfo> found;
        userTree.findAll(key, found);
        if (found.empty()) return -1; // 用户名不存在
        if (strcmp(password, found[0].password) != 0) return -1; // 密码不对

        loggedIn.insert({key, found[0].privilege});
        return 0;
    }

    int logout(const char *username) {
        Username key(username);
        auto it = loggedIn.find(key);
        if (it == loggedIn.end()) return -1;
        loggedIn.erase(it);
        return 0;
    }

    int queryProfile(const char *cur_username, const char *username, UserInfo &outInfo) {
        Username curKey(cur_username);
        auto it = loggedIn.find(curKey);
        if (it == loggedIn.end()) return -1;

        Username key(username);
        sjtu::vector<UserInfo> found;
        userTree.findAll(key, found);
        if (found.empty()) return -1;

        bool sameUser = (strcmp(cur_username, found[0].username) == 0);
        if (!sameUser && it->second <= found[0].privilege) return -1;

        outInfo = found[0];
        return 0;
    }

    // newPassword/newName/newMailAddr传nullptr表示不修改，newPrivilege传-1表示不修改
    int modifyProfile(const char *cur_username, const char *username,
                      const char *newPassword, const char *newName, const char *newMailAddr, int newPrivilege, UserInfo &outInfo) {
        Username curKey(cur_username);
        auto it = loggedIn.find(curKey);
        if (it == loggedIn.end()) return -1;

        Username targetKey(username);
        sjtu::vector<UserInfo> found;
        userTree.findAll(targetKey, found);
        if (found.empty()) return -1;

        bool sameUser = (strcmp(cur_username, found[0].username) == 0);
        if (!sameUser && it->second <= found[0].privilege) return -1;
        if (newPrivilege != -1 && it->second <= newPrivilege) return -1;

        UserInfo newUser = found[0];
        if (newPassword) {
            strncpy(newUser.password, newPassword, 30);
            newUser.password[30] = '\0';
        }
        if (newName) {
            strncpy(newUser.name, newName, 50);
            newUser.name[50] = '\0';
        }
        if (newMailAddr) {
            strncpy(newUser.mailAddr, newMailAddr, 30);
            newUser.mailAddr[30] = '\0';
        }
        if (newPrivilege != -1) {
            newUser.privilege = newPrivilege;
        }

        userTree.erase(targetKey, found[0]);
        userTree.insert(targetKey, newUser);

        auto newIt = loggedIn.find(targetKey);
        if (newIt != loggedIn.end() && newPrivilege != -1) newIt->second = newPrivilege;

        outInfo = newUser;
        return 0;
    }

    // exit中用
    void logoutAll() {
        loggedIn.clear();
    }
    // clean中用，文件在main里面删
    void resetMemory() {
        loggedIn.clear();
        userCount = 0;
    }

    bool isLoggedIn(const char *username) {
        Username key(username);
        auto it = loggedIn.find(key);
        return it != loggedIn.end();
    }
};

#endif //TICKETSYSTEM_USERMANAGER_H