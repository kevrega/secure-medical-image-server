#ifndef USER_H
#define USER_H

#include <string>

class User {
public:
    User(int id, std::string const& username, std::string const& role);

    int get_id() const;
    std::string get_username() const;
    std::string get_role() const;

private:
    int id;
    std::string username;
    std::string role;
};

#endif