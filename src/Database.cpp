#include "Database.h"

#include <stdexcept>
#include <string>

// -------------------
// CONSTRUCTOR/
// DESTRUCTOR
// -------------------

Database::Database(std::string const& filename)
    : db{nullptr} {
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
    // Creates file if filename doesn't exist OR opens it if it already exists
    // Give SQLite the filename in c-string format and store databse in db
        std::string error_message{sqlite3_errmsg(db)};

        sqlite3_close(db);
        db = nullptr;

        throw std::runtime_error{
            "Could not open database: " + error_message
        };
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    // Enforce foreign key relationships declared
}

Database::~Database() {
    if (db != nullptr) {
    // If db points to open database connection
        sqlite3_close(db);
    }
}

// -------------------
// CREATE TABLES
// -------------------

void Database::createTables() {
    //SQLite expects C-string
    const char* sql =
        "CREATE TABLE IF NOT EXISTS patients ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "age INTEGER NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY,"
        "username TEXT NOT NULL,"
        "role TEXT NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS studies ("
        "id INTEGER PRIMARY KEY,"
        "patient_id INTEGER NOT NULL,"
        "description TEXT NOT NULL,"
        "FOREIGN KEY(patient_id) REFERENCES patients(id)"
        ");";

    char* error_message = nullptr;

    if (sqlite3_exec(db, sql, nullptr, nullptr, &error_message) != SQLITE_OK)
    // nullptr are for features we don't need as of now
    {
        std::string error{error_message};

        sqlite3_free(error_message);
        // No longer need SQLite's memory so we free it

        throw std::runtime_error {
            "Could not create tables: " + error
        };
    }
}

// -------------------
// PATIENTS
// -------------------

void Database::addPatient(int id, std::string const& name, int age) {
    const char* sql =
        "INSERT INTO patients (id, name, age) "
        "VALUES (?, ?, ?);"; //placeholder

    sqlite3_stmt* statement = nullptr;
    // Prepare SQL statement

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
    // db = open db con, sql = sql command as text, -1 = read until null terminator,
    // &statement = store the statement here
        throw std::runtime_error {
            "Could not prepare insert statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);
    sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    // SQLITE_TRANSIENT: safely copy text because orginal string might change
    sqlite3_bind_int(statement, 3, age);

    if (sqlite3_step(statement) != SQLITE_DONE) { // Execute the prepared SQL statement
        sqlite3_finalize(statement); // Release prepared statement from memory

        throw std::runtime_error{
            "Could not insert patient"
        };
    }

    sqlite3_finalize(statement);
}

std::vector<Patient> Database::getPatients() {
    std::vector<Patient> patients;

    const char* sql =
        "SELECT id, name, age FROM patients;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare select statement"
        };
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        // Run until there's no more rows
        int id = sqlite3_column_int(statement, 0);

        std::string name{
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            )
        };
        // Convert to normal string from C-string

        int age = sqlite3_column_int(statement, 2);

        patients.push_back(Patient{id, name, age});
    }

    sqlite3_finalize(statement);

    return patients;
}

void Database::updatePatient(int id, std::string const& name, int age) {
    const char* sql =
        "UPDATE patients "
        "SET name = ?, age = ? "
        "WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare update statement"
        };
    }

    sqlite3_bind_text(statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 2, age);
    sqlite3_bind_int(statement, 3, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not update patient: " + error
        };
    }

    sqlite3_finalize(statement);
}

void Database::deletePatient(int id) {
    const char* sql =
        "DELETE FROM patients "
        "WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare delete statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not delete patient: " + error
        };
    }

    sqlite3_finalize(statement);
}

// -------------------
// USERS
// -------------------

void Database::addUser(int id, std::string const& username, std::string const& role) {
    const char* sql =
        "INSERT INTO users (id, username, role) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare user insert statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);
    sqlite3_bind_text(statement, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, role.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not insert user: " + error
        };
    }

    sqlite3_finalize(statement);
}

std::vector<User> Database::getUsers()
{
    std::vector<User> users;

    const char* sql =
        "SELECT id, username, role FROM users;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare user select statement"
        };
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        int id = sqlite3_column_int(statement, 0);

        std::string username{
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            )
        };

        std::string role{
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            )
        };

        users.push_back(User{id, username, role});
    }

    sqlite3_finalize(statement);

    return users;
}

void Database::updateUser(int id, std::string const& username, std::string const& role)
{
    const char* sql =
        "UPDATE users "
        "SET username = ?, role = ? "
        "WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare user update statement"
        };
    }

    sqlite3_bind_text(statement, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not update user: " + error
        };
    }

    sqlite3_finalize(statement);
}

void Database::deleteUser(int id) {
    const char* sql =
        "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare user delete statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not delete user: " + error
        };
    }

    sqlite3_finalize(statement);
}

// -------------------
// STUDIES
// -------------------

void Database::addStudy(int id, int patient_id, std::string const& description) {
    const char* sql =
        "INSERT INTO studies (id, patient_id, description) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error{
            "Could not prepare study insert statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);
    sqlite3_bind_int(statement, 2, patient_id);
    sqlite3_bind_text(statement, 3, description.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not insert study: " + error
        };
    }

    sqlite3_finalize(statement);
}

std::vector<Study> Database::getStudies() {
    std::vector<Study> studies;

    const char* sql =
        "SELECT id, patient_id, description FROM studies;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare study select statement"
        };
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        int id = sqlite3_column_int(statement, 0);
        int patient_id = sqlite3_column_int(statement, 1);

        std::string description{
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            )
        };

        studies.push_back(Study{id, patient_id, description});
    }

    sqlite3_finalize(statement);

    return studies;
}

void Database::updateStudy(int id, int patient_id, std::string const& description) {
    const char* sql =
        "UPDATE studies "
        "SET patient_id = ?, description = ? "
        "WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare study update statement"
        };
    }

    sqlite3_bind_int(statement, 1, patient_id);
    sqlite3_bind_text(statement, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not update study: " + error
        };
    }

    sqlite3_finalize(statement);
}

void Database::deleteStudy(int id)
{
    const char* sql =
        "DELETE FROM studies WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error{
            "Could not prepare study delete statement"
        };
    }

    sqlite3_bind_int(statement, 1, id);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::string error{sqlite3_errmsg(db)};
        sqlite3_finalize(statement);

        throw std::runtime_error{
            "Could not delete study: " + error
        };
    }

    sqlite3_finalize(statement);
}
