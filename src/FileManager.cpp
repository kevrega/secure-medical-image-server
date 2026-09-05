#include "FileManager.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

FileManager::FileManager(std::string const& filename)
    : filename{filename} {}

void FileManager::save_patient(Patient const& patient) const {
    std::ofstream file{filename, std::ios::app}; 
    // Write to file, append to avoid risking replacing
    // Creates file automatically as long as "data/" exists  

    if (!file) {
        throw std::runtime_error{"Could not open file for writing"};
    }

    file << patient.get_id() << ' '
         << patient.get_name() << ' '
         << patient.get_age() << '\n';

    if (!file) {
        throw std::runtime_error{"Failed to write patient data"};
    }
}

std::vector<Patient> FileManager::load_patients() const {
    std::vector<Patient> patients;

    std::ifstream file{filename};

    if (!file) {
        return patients;
    }

    int id;
    std::string name;
    int age;

    while (file >> id >> name >> age) {
        Patient patient{id, name, age};
        patients.push_back(patient);
    }

    if (file.bad()) {
        throw std::runtime_error{"Failed while reading patient file"};
    }

    if (!file.eof()) {
        throw std::runtime_error{"Malformed patient data"};
    }

    return patients;
}