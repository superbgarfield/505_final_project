//
// Created by cis505 on 11/19/19.
//

#ifndef FINAL_MAIL_H
#define FINAL_MAIL_H


#include <string>
#include <vector>

class Mail {

    std::string sender;
    std::vector<std::string> recipients;
    std::string subject;
    std::string time;
    std::string user;
public:
    const std::string &getSender() const;

    const std::vector<std::string> &getRecipients() const;

    const std::string &getSubject() const;

    const std::string &getTime() const;

    const std::string &getUser() const;

    const std::string &getContent() const;

private:
    std::string content;
public:
    Mail(const std::string &sender, const std::vector<std::string> &recipients, const std::string &subject,
         const std::string &time, const std::string &user, const std::string &content);
};


#endif //FINAL_MAIL_H
