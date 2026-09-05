#include <iostream>
#include <vector>

#include "Patient.h"
#include "FileManager.h"

int main() {
    FileManager manager{"data/patients.txt"};

    Patient patient{1, "John_Doe", 45};

    manager.save_patient(patient);

    std::vector<Patient> patients{manager.load_patients()};

    for (Patient const& p : patients) {
        std::cout << p.get_id() << " "
                  << p.get_name() << " "
                  << p.get_age() << '\n';
    }

    return 0;
}