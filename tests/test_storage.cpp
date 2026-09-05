#include <gtest/gtest.h>
#include "FileManager.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

TEST(FileManagerTest, SavesAndLoadsPatient) {
    std::string filename{"test_patients.txt"};

    // Make sure an old test file does not affect the test.
    std::remove(filename.c_str());

    FileManager manager{filename};

    Patient patient{1, "John_Doe", 45};

    manager.save_patient(patient);

    std::vector<Patient> patients{manager.load_patients()};

    ASSERT_EQ(patients.size(), 1);

    EXPECT_EQ(patients[0].get_id(), 1);
    EXPECT_EQ(patients[0].get_name(), "John_Doe");
    EXPECT_EQ(patients[0].get_age(), 45);

    // Clean up after the test.
    std::remove(filename.c_str());
}


TEST(FileManagerTest, SavesMultiplePatients) {
    std::string filename{"test_patients_multiple.txt"};

    std::remove(filename.c_str());

    FileManager manager{filename};

    manager.save_patient(Patient{1, "John_Doe", 45});
    manager.save_patient(Patient{2, "Jane_Doe", 37});

    std::vector<Patient> patients{manager.load_patients()};

    ASSERT_EQ(patients.size(), 2);

    EXPECT_EQ(patients[0].get_id(), 1);
    EXPECT_EQ(patients[0].get_name(), "John_Doe");
    EXPECT_EQ(patients[0].get_age(), 45);

    EXPECT_EQ(patients[1].get_id(), 2);
    EXPECT_EQ(patients[1].get_name(), "Jane_Doe");
    EXPECT_EQ(patients[1].get_age(), 37);

    std::remove(filename.c_str());
}

TEST(FileManagerTest, ReturnsEmptyForMissingFile) {
    std::string filename{"file_that_does_not_exist.txt"};

    std::remove(filename.c_str());

    FileManager manager{filename};

    std::vector<Patient> patients{manager.load_patients()};

    EXPECT_TRUE(patients.empty());
}


TEST(FileManagerTest, ThrowsForMalformedPatientData) {
    std::string filename{"malformed_patients.txt"};

    {
        std::ofstream file{filename};
        file << "1 John_Doe banana\n";
    }

    FileManager manager{filename};

    EXPECT_THROW(
        manager.load_patients(),
        std::runtime_error
    );

    std::remove(filename.c_str());
}


TEST(FileManagerTest, ThrowsWhenFileCannotBeOpenedForWriting) {
    FileManager manager{
        "directory_that_does_not_exist/patients.txt"
    };

    Patient patient{1, "John_Doe", 45};

    EXPECT_THROW(
        manager.save_patient(patient),
        std::runtime_error
    );
}