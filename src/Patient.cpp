#include "Patient.h"

Patient::Patient(int id, std::string const& name, int age)
    : id{id}, name{name}, age{age} {}

int Patient::get_id() const {
    return id;
}

std::string Patient::get_name() const {
    return name;
}

int Patient::get_age() const {
    return age;
}