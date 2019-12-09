//
// Created by cis505 on 11/17/19.
//

#include "HTMLtoString.h"
#include <fstream>
#include <string>
#include <iostream>
#include <random>

std::string HTMLtoString::goodRequest(const std::string& page, bool allocateCookie) {
    std::string htmlPage = readHTML(pages[page]);
    std::string cookie;
    //set cookie
    if (allocateCookie) {
        std::random_device randomDevice;
        cookie = "Set-Cookie: penncloud-session=" + std::to_string(randomDevice()) + "\r\n";
        //TODO: put in BigTable
    }
    std::string response = "HTTP/1.1 200 Okay\r\nContent-Type: text/html\r\nContent-length: " + std::to_string(htmlPage.length()) + "\r\n" + cookie + "\r\n";
    response.append(htmlPage);
    return response;
}

std::string HTMLtoString::readHTML(const std::string& path) {
    std::string data;
    std::ifstream in(path.c_str());
    getline(in, data, std::string::traits_type::to_char_type(
            std::string::traits_type::eof()));
    return data;
}

std::string HTMLtoString::badRequest() {
    return "";
}
