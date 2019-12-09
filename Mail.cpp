//
// Created by cis505 on 11/19/19.
//

#include "Mail.h"

Mail::Mail(const std::string &sender, const std::vector<std::string> &recipients, const std::string &subject,
           const std::string &time, const std::string &user, const std::string &content) : sender(sender),
                                                                                           recipients(recipients),
                                                                                           subject(subject), time(time),
                                                                                           user(user),
                                                                                           content(content) {}

const std::string &Mail::getSender() const {
    return sender;
}

const std::vector<std::string> &Mail::getRecipients() const {
    return recipients;
}

const std::string &Mail::getSubject() const {
    return subject;
}

const std::string &Mail::getTime() const {
    return time;
}

const std::string &Mail::getUser() const {
    return user;
}

const std::string &Mail::getContent() const {
    return content;
}
