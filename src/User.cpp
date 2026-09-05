#include "User.h"

User::User(int id, std::string username, std::string role)
    : id{id}, username{username}, role{role} {}

int User::get_id() const {
    return id;
}

std::string User::get_username() const {
    return username;
}

std::string User::get_role() const {
    return role;
}