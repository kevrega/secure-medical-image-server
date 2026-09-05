#ifndef STUDY_H
#define STUDY_H

#include <string>

class Study {
public:
    Study(int id, int patient_id, std::string const& description);

    int get_id() const;
    int get_patient_id() const;
    std::string get_description() const;

private:
    int id;
    int patient_id;
    std::string description;
};

#endif