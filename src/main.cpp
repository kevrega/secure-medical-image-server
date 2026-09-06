#include <iostream>

#include "Database.h"

int main() {
    Database db{"data/medical.db"};
    db.createTables();

    auto patients = db.getPatients();

    for (auto const& patient : patients) {
        std::cout
            << patient.get_id() << " "
            << patient.get_name() << " "
            << patient.get_age() << '\n';
    }

    return 0;
}