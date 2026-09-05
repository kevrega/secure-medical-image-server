#ifndef PATIENT_H
#define PATIENT_H

#include <string>

class Patient {
public:
    Patient(int id, std::string name, int age);

    int get_id() const;
    std::string get_name() const;
    int get_age() const;

private:
    int id;
    std::string name;
    int age;
};

#endif