#include "Study.h"

Study::Study(int id, int patient_id, std::string description)
    : id{id}, patient_id{patient_id}, description{description} {}

int Study::get_id() const {
    return id;
}

int Study::get_patient_id() const {
    return patient_id;
}

std::string Study::get_description() const {
    return description;
}