//
// Created by cis505 on 11/19/19.
//

#ifndef FINAL_SERVER_H
#define FINAL_SERVER_H


#include <map>

class Server {

public:
    static std::map<std::string, std::string> user_pass;
    static std::map<std::string, std::string> guid_user;
    bool run = true;
    void startServer();
};


#endif //FINAL_SERVER_H
