#include <gtest/gtest.h>

#include <stdexcept>

#include "Database.h"

// -------------------------
// Patient tests 
// -------------------------

TEST(DatabaseTest, AddAndGetPatient) {
    Database db{":memory:"};
    db.createTables();

    db.addPatient(1, "Kevin", 24);

    auto patients = db.getPatients();

    ASSERT_EQ(patients.size(), 1);

    EXPECT_EQ(patients[0].get_id(), 1);
    EXPECT_EQ(patients[0].get_name(), "Kevin");
    EXPECT_EQ(patients[0].get_age(), 24);
}

TEST(DatabaseTest, UpdatePatient) {
    Database db{":memory:"};
    db.createTables();

    db.addPatient(1, "Kevin", 24);

    db.updatePatient(1, "Kevin_Updated", 25);

    auto patients = db.getPatients();

    ASSERT_EQ(patients.size(), 1);

    EXPECT_EQ(patients[0].get_name(), "Kevin_Updated");
    EXPECT_EQ(patients[0].get_age(), 25);
}

TEST(DatabaseTest, DeletePatient) {
    Database db{":memory:"};
    db.createTables();

    db.addPatient(1, "Kevin", 24);

    db.deletePatient(1);

    auto patients = db.getPatients();

    EXPECT_TRUE(patients.empty());
}

TEST(DatabaseTest, DuplicatePatientIdThrows) {
    Database db{":memory:"};
    db.createTables();

    db.addPatient(1, "Kevin", 24);

    EXPECT_THROW(
        db.addPatient(1, "Patient", 30),
        std::runtime_error
    );
}

// -------------------------
// User tests 
// -------------------------

TEST(DatabaseTest, AddAndGetUser) {
    Database db{":memory:"};
    db.createTables();

    db.addUser(1, "kevin", "admin");

    auto users = db.getUsers();

    ASSERT_EQ(users.size(), 1);

    EXPECT_EQ(users[0].get_id(), 1);
    EXPECT_EQ(users[0].get_username(), "kevin");
    EXPECT_EQ(users[0].get_role(), "admin");
}

// -------------------------
// Study tests 
// -------------------------

TEST(DatabaseTest, AddAndGetStudy) {
    Database db{":memory:"};
    db.createTables();

    // Patient must exist because of foreign key in the next row
    db.addPatient(1, "Kevin", 24);

    db.addStudy(10, 1, "Brain MRI");

    auto studies = db.getStudies();

    ASSERT_EQ(studies.size(), 1);

    EXPECT_EQ(studies[0].get_id(), 10);
    EXPECT_EQ(studies[0].get_patient_id(), 1);
    EXPECT_EQ(studies[0].get_description(), "Brain MRI");
}

TEST(DatabaseTest, InvalidPatientForStudyThrows) {
    Database db{":memory:"};
    db.createTables();

    // Patient 999 does not exist
    EXPECT_THROW(
        db.addStudy(10, 999, "Brain MRI"),
        std::runtime_error
    );
}