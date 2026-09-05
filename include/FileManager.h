#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

#include "Patient.h"

class FileManager {
public:
    FileManager(std::string const& filename);

    void save_patient(Patient const& patient) const;

    std::vector<Patient> load_patients() const;

private:
    std::string filename;
};

#endif