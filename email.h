//
// Created by Kanika Prasad Nadkarni on 11/19/19.
//
#include<vector>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <set>

class email {
public:
    std::string sender;
    std::string timestamp;
    std::vector <std::string>recepients;
    std::string content;
    std::string subject;
    std::string hashValueString;
    //std::string username;
};
