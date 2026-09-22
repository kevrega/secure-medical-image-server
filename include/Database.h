#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>

#include "Patient.h"
#include "User.h"
#include "Study.h"


// Represents one medical image stored for a study
struct MedicalImage {
    int id;
    int study_id;
    std::string filename;
    std::string storage_path;
};


class Database {
public:
    Database(std::string const& filename);
    ~Database();

    void createTables();

    //Patients
    void addPatient(int id, std::string const& name, int age);
    std::vector<Patient> getPatients();
    void updatePatient(int id, std::string const& name, int age);
    void deletePatient(int id);

    //Users
    void addUser(int id, std::string const& username, std::string const& role);
    std::vector<User> getUsers();
    void updateUser(int id, std::string const& username, std::string const& role);
    void deleteUser(int id);

    //Studies
    void addStudy(int id, int patient_id, std::string const& description);
    std::vector<Study> getStudies();
    void updateStudy(int id, int patient_id, std::string const& description);
    void deleteStudy(int id);

    //Medical images
    int addImage(
        int study_id,
        std::string const& filename,
        std::string const& storage_path
    );

    std::vector<MedicalImage> getImages(int study_id);

    void deleteImage(
        int id,
        int study_id
    );

private:
    sqlite3* db;
};

#endif