#include "FileManager.h"

#include <fstream>
#include <sstream>

FileManager::FileManager(std::string filename)
    : filename{filename} {}

void FileManager::save_patient(Patient const& patient) const {
    std::ofstream file{filename, std::ios::app}; 
    // Write to file, append to avoid risking replacing

    if (!file) { // More error-handling in future
        return;
    }

    file << patient.get_id() << ' '
         << patient.get_name() << ' '
         << patient.get_age() << '\n';
}

std::vector<Patient> FileManager::load_patients() const {
    std::vector<Patient> patients;

    std::ifstream file{filename};

    if (!file) { // More error-handling in future
        return patients;
    }

    int id;
    std::string name;
    int age;

    while (file >> id >> name >> age) {
        Patient patient{id, name, age};
        patients.push_back(patient);
    }

    return patients;
}
