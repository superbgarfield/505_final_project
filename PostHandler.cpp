//
// Created by cis505 on 11/19/19.
//

#include <iostream>
#include "PostHandler.h"
#include "json.hpp"
#include "Server.h"
#include "Mail.h"
#include "DriveClient.h"
#include "userHandler.h"
#include "admin.h"

using json = nlohmann::json;

DriveClient driveClient;

std::string PostHandler::handleRequest(std::string& request) {
    //eliminate POST
    request = request.substr(request.find_first_of('/'));
    path = request.substr(0,request.find_first_of(' '));
    std::string cookie = findCookie(request);
    if (path == registerPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string user = j["email"];
        std::string pass = j["password"];
        //this means user already exists
        bool valid = store_usr_pass(Server::bigTableClient, user, pass, false);
        if (valid) {
            json p;
            p["message"] = "PASS";
            return sendOk(p.dump());
        } else {
            json fail;
            fail["message"] = "FAIL";
            return sendOk(fail.dump());
        }
    } else if (path == loginPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string user = j["email"];
        std::string pass = j["password"];
        //Potentially could fail if backend goes down
        store_usr_cookie(Server::bigTableClient, user, cookie);
        bool valid = usr_valid(Server::bigTableClient, user, pass);
        if (valid) {
            json p;
            p["message"] = "PASS";
            return sendOk(p.dump());
        } else {
            json fail;
            fail["message"] = "FAIL";
            return sendOk(fail.dump());
        }
    } else if (path == logOutPath) {
        del_usr_cookie(Server::bigTableClient, cookie);
        json p;
        p["message"] = "PASS";
        return p.dump();
    } else if (path == passwordPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string pass = j["password"];
        bool valid = store_usr_pass(Server::bigTableClient, get_usr_from_cookie(Server::bigTableClient, cookie), pass, true);
        if (valid) {
            json p;
            p["message"] = "PASS";
            return sendOk(p.dump());
        } else {
            json fail;
            fail["message"] = "FAIL";
            return sendOk(fail.dump());
        }
    } else if (path == mailPath) {
        //TODO: modify this to work with MailClient
        std::vector<std::string> rcpts;
        Mail m1("leo@penncloud", rcpts, "Testing the email", "11/19/2019 15:55:10", Server::user_pass[Server::guid_user[cookie]], "Sending a new email!");
        Mail m2("kanika@penncloud", rcpts, "Does it still work?", "11/20/2019 08:10:00", Server::user_pass[Server::guid_user[cookie]], "Email 2.");
        Mail m3("chris@penncloud", rcpts, "This is fun", "11/21/2019 12:12:12", Server::user_pass[Server::guid_user[cookie]], "Last one.");
        std::vector<Mail> mails;
        mails.push_back(m1);
        mails.push_back(m2);
        mails.push_back(m3);
        json mailList;
        for (const auto& mail : mails) {
            json m;
            m["sender"] = mail.getSender();
            m["subject"] = mail.getSubject();
            m["time"] = mail.getTime();
            m["content"] = mail.getContent();
            mailList.push_back(m);
        }
        return sendOk(mailList.dump());
    } else if (path == mailSendPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string sender = Server::guid_user[cookie];
        std::string subject = j["subject"];
        std::string recipientsString = j["recipients"];
        std::vector<std::string> recipients = parseRecipients(recipientsString);
        std::string content = j["content"];
        time_t rawtime;
        struct tm * timeinfo;
        char buffer [80];
        time (&rawtime);
        timeinfo = localtime (&rawtime);
        strftime (buffer,80,"%x %X",timeinfo);
        std::string time(buffer);
        Mail m(sender, recipients, subject, time, sender, content);
        //TODO: pass to MailClient
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == mailDeletePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string user = Server::guid_user[cookie];
        std::string sender = j["sender"];
        std::string subject = j["subject"];
        std::string time = j["time"];
        Mail m(sender, {}, subject, time, user, "");
        //TODO: pass to MailClient
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == drivePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string reqPath = j["current"];
        //TODO: get username
        driveClient.initialize("fake");
        std::map<std::string, std::vector<std::string>> current_contents;
        std::string status = driveClient.display(reqPath, current_contents);
        if (status == BACKEND_DEAD) {
            //TODO: return 500
        }
        json ret;
        ret["files"] = current_contents[TYPE_FILE];
        ret["folders"] = current_contents[TYPE_DIRECTORY];
        return sendOk(ret.dump());
    } else if (path == driveDeletePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string type = j["type"];
        std::string deletePath = j["name"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string status = driveClient.remove(currentDir, deletePath, type == "file" ? TYPE_FILE : TYPE_DIRECTORY);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == driveMovePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string currentName = j["current-name"];
        std::string type = j["type"];
        std::string newPath = j["new-path"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string status = driveClient.move(currentDir, currentName, newPath, type == "file" ? TYPE_FILE : TYPE_DIRECTORY);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        } else if (status == DUPLICATE_NAME) {
            //TODO: name already exists
        } else if (status == DIRECTORY_NOT_EXIST) {
            //TODO: invalid dir
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == driveRenamePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string currentName = j["current-name"];
        std::string type = j["type"];
        std::string newName = j["new-name"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string status = driveClient.rename(currentDir, currentName, newName, type == "file" ? TYPE_FILE : TYPE_DIRECTORY);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        } else if (status == DUPLICATE_NAME) {
            //TODO: name already exists
        } else if (status == FORBIDDEN_CHARS) {
            //TODO:
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == driveNewFolderPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string newName = j["folder-name"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string status = driveClient.make_dir(currentDir, newName);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        } else if (status == DUPLICATE_NAME) {
            //TODO: name already exists
        } else if (status == FORBIDDEN_CHARS) {
            //TODO:
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == driveUploadPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string fileName = j["name"];
        std::string content = j["content"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string status = driveClient.upload(currentDir, fileName, content);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        } else if (status == DUPLICATE_NAME) {
            //TODO: name already exists
        } else if (status == FORBIDDEN_CHARS) {
            //TODO:
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == driveDownloadPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string currentDir = j["current"];
        std::string fileName = j["name"];
        std::string user = get_usr_from_cookie(Server::bigTableClient, cookie);
        driveClient.initialize(user);
        std::string content;
        std::string status = driveClient.download(currentDir, fileName, content);
        if (status == BACKEND_DEAD) {
            //TODO: return 500;
        }
        json ret;
        ret["file"] = content;
        return sendOk(ret.dump());
    } else if (path == adminPath) {
        std::vector<ServerInfo> server_info = get_back_nodes(Server::bigTableClient);
        json ret;
        for (auto server: server_info) {
            json serv;
            serv["id"] = server.id;
            serv["address"] = server.address;
            serv["load"] = server.load;
            serv["status"] = server.on;
            ret.push_back(serv);
        }
        //TODO: frontend servers
        return sendOk(ret.dump());
    } else if (path == adminTogglePath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string status = j["status"];
        std::string address = j["address"];
        if (status == "on") {
            turn_off_node(address);
        } else {
            turn_on_node(address);
        }
        json p;
        p["message"] = "PASS";
        return sendOk(p.dump());
    } else if (path == adminContentPath) {
        std::string data = findData(request);
        data = data.substr(0, data.find_last_of('}') + 1);
        json j = json::parse(data);
        std::string address = j["address"];
        std::vector<ServerDataPoint> serverData = get_node_data(address);
        json ret;
        for (auto data: serverData) {
            json dataPoint;
            dataPoint["row"] = data.row;
            dataPoint["col"] = data.col;
            dataPoint["data"] = data.partialData;
        }
        return sendOk(ret.dump());
    }
    //should be unreachable
    return "";
}

std::string PostHandler::findData(std::string& request) {
    std::string lineEnd = "\r\n";
    int pos = 0;
    std::string headerLine;
    while ((pos = request.find(lineEnd)) != std::string::npos) {
        headerLine = request.substr(0, pos);
        request.erase(0, pos + lineEnd.length());
        if (headerLine.empty()) {
            break;
        }
    }
    return request;
}

std::string PostHandler::findCookie(std::string request) {
    std::string lineEnd = "\r\n";
    int pos = 0;
    std::string headerLine;
    while ((pos = request.find(lineEnd)) != std::string::npos) {
        headerLine = request.substr(0, pos);
        request.erase(0, pos + lineEnd.length());
        if (headerLine.substr(0, headerLine.find_first_of(':')) == "Cookie") {
            return headerLine.substr(headerLine.find_first_of(':') + 1);
        }
    }
    return "";
}

std::vector<std::string> PostHandler::parseRecipients(std::string recipients) {
    std::vector<std::string> ret;
    int pos = 0;
    std::string rec;
    while ((pos = recipients.find(", ")) != std::string::npos) {
        rec = recipients.substr(0, pos);
        ret.push_back(rec);
        recipients.erase(0, pos + 2);
    }
    ret.push_back(recipients);
    return ret;
}

std::string PostHandler::sendOk(const std::string& data) {
    return "HTTP/1.1 200 Okay\r\nContent-Type: application/json\r\nContent-length: " + std::to_string(data.length()) + "\r\n\r\n" + data + "\r\n";
}

