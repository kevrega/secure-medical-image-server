#include "TcpServer.h"
#include "Database.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <fstream>
#include <thread>
#include <cstdlib> // system()

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


// Create a simple HTTP response
std::string makeHttpResponse(
    std::string const& status,
    std::string const& body
) {
    return
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
}


TcpServer::TcpServer(int port)
    : port{port},
      server_socket{-1} {}


TcpServer::~TcpServer() {

    if (server_socket != -1) {

        // If we have an open server socket, close it when we are done
        close(server_socket);
    }
}


void TcpServer::start() {

    // Create TCP, IPv4 socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        throw std::runtime_error{"Socket failed"};
    }


    // Allow the server to reuse the same local address/port after restarting
    int option = 1;

    // Change a setting on the socket
    setsockopt(
        server_socket,       // Which socket to change
        SOL_SOCKET,          // Socket-level option
        SO_REUSEADDR,        // Allow address/port reuse
        &option,             // Turn option on
        sizeof(option)       // Size of option
    );


    // Configure socket address so socket knows where to listen
    sockaddr_in address{};

    address.sin_family = AF_INET;              // IPv4
    address.sin_port = htons(port);            // Port to listen on
    address.sin_addr.s_addr = INADDR_ANY;      // Accept connections on any local IP


    int bind_result = bind(
        server_socket,                         // Which socket?
        reinterpret_cast<sockaddr*>(&address), // Which address (IP + port)?
                                               // bind() expects sockaddr*
        sizeof(address)                        // Number of bytes in address
    );


    if (bind_result == -1) {

        // 0 = Success, -1 = Failure
        throw std::runtime_error{"Bind failed"};
    }


    if (listen(server_socket, 5) == -1) {

        // Keep a waiting queue for incoming connections
        // Up to 5 connections can wait in the queue
        throw std::runtime_error{"Listen failed"};
    }


    std::cout << "Listening on port " << port << '\n';


    while (true) {

        // WAIT here until a client connects
        // accept() creates a new client socket for that connection
        int client_socket =
            accept(server_socket, nullptr, nullptr);


        if (client_socket == -1) {

            // Socket ID >= 0 is valid
            throw std::runtime_error{"Accept failed"};
        }


        std::cout << "Client connected\n";


        // Create a new thread for this client
        std::thread client_thread(
            &TcpServer::handleClient, // Run this function
            this, // On this Tcpservr object
            client_socket // With this argument
        );


        // Let this client thread run by itself
        // Server can immediately go back to accept() for another client
        client_thread.detach();
    }
}


// Handles one connected client
void TcpServer::handleClient(int client_socket) {

    // Each thread gets its own database connection
    Database db{"data/medical.db"};
    db.createTables();


    // Buffer where received client data will be stored
    char buffer[1024]{};


    // Receive data from this client
    ssize_t bytes_received = recv(
        client_socket,         // Receive from specific client
        buffer,                // Store data here
        sizeof(buffer) - 1,    // Max data to receive
        0
    );


    if (bytes_received > 0) {

        std::string message{buffer};

        std::cout << "Received:\n"
                  << message
                  << '\n';

        std::string response;


        // ----------------------
        // HTTP / REST REQUESTS
        // ----------------------

        bool is_http =
            message.find("GET ") == 0 ||
            message.find("POST ") == 0 ||
            message.find("PUT ") == 0 ||
            message.find("DELETE ") == 0;


        if (is_http) {

            // First line looks like:
            // GET /patients HTTP/1.1
            std::istringstream request{message};

            std::string method;
            std::string path;
            std::string http_version;

            request >> method >> path >> http_version;


            // Find HTTP body (everything after empty line)
            std::string body;

            std::size_t body_position =
                message.find("\r\n\r\n");


            // If "\r\n\r\n" was found,
            // there is a body after the HTTP headers
            if (body_position != std::string::npos) {

                // Take everything after "\r\n\r\n"
                // and store it as the request body
                body = message.substr(body_position + 4);
            }


            // Reject request if the body is too large
            if (body.size() > 512) {

                response = makeHttpResponse(
                    "413 Payload Too Large",
                    "Request body is too large\n"
                );

                send(
                    client_socket,
                    response.c_str(),
                    response.size(),
                    0
                );

                close(client_socket);
                return;
            }


            // ----------------------
            // BASIC SECURITY
            // ----------------------

            // Get the expected API key from an environment variable
            const char* expected_key =
                std::getenv("MEDICAL_API_KEY");


            // Find the API key sent by the client
            std::string api_key;

            std::string key_header = "X-API-Key: ";

            std::size_t key_position = message.find(key_header);

            if (key_position != std::string::npos) {

                // Start reading after "X-API-Key: "
                std::size_t key_start = key_position + key_header.size();

                // Find the end of this HTTP header
                std::size_t key_end = message.find("\r\n", key_start);

                // Extract only the API key
                api_key =
                    message.substr(
                        key_start,
                        key_end - key_start
                    );
            }


            // Server must have an API key configured
            if (expected_key == nullptr) {

                response = makeHttpResponse(
                    "500 Internal Server Error",
                    "Server API key is not configured\n"
                );
            }


            // Client must send the correct API key
            else if (api_key != expected_key) {

                response = makeHttpResponse(
                    "401 Unauthorized",
                    "Invalid or missing API key\n"
                );
            }


            // API key is correct, continue to the REST API
            else {

                try {

                // ----------------------
                // PATIENT REST API
                // ----------------------

                // GET /patients
                if (
                    method == "GET" &&
                    path == "/patients"
                ) {

                    auto patients = db.getPatients();

                    std::string result;


                    for (auto const& patient : patients) {

                        result +=
                            std::to_string(patient.get_id()) +
                            " " +
                            patient.get_name() +
                            " " +
                            std::to_string(patient.get_age()) +
                            "\n";
                    }


                    if (patients.empty()) {
                        result = "No patients\n";
                    }


                    response = makeHttpResponse(
                        "200 OK",
                        result
                    );
                }


                // POST /patients
                // Body: 2 John 30
                else if (
                    method == "POST" &&
                    path == "/patients"
                ) {

                    std::istringstream data{body};

                    int id;
                    std::string name;
                    int age;


                    if (
                        data >> id >> name >> age &&
                        id > 0 &&
                        !name.empty() &&
                        name.size() <= 50 &&
                        age >= 0 &&
                        age <= 120
                    ) {

                        db.addPatient(
                            id,
                            name,
                            age
                        );


                        response = makeHttpResponse(
                            "201 Created",
                            "Patient added\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid patient data\n"
                        );
                    }
                }


                // PUT /patients
                // Body: 2 John_Updated 31
                else if (
                    method == "PUT" &&
                    path == "/patients"
                ) {

                    std::istringstream data{body};

                    int id;
                    std::string name;
                    int age;


                    if (
                        data >> id >> name >> age &&
                        id > 0 &&
                        !name.empty() &&
                        name.size() <= 50 &&
                        age >= 0 &&
                        age <= 120
                    ) {

                        db.updatePatient(
                            id,
                            name,
                            age
                        );


                        response = makeHttpResponse(
                            "200 OK",
                            "Patient updated\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid patient data\n"
                        );
                    }
                }


                // DELETE /patients
                // Body: 2
                else if (
                    method == "DELETE" &&
                    path == "/patients"
                ) {

                    std::istringstream data{body};

                    int id;


                    if (
                        data >> id &&
                        id > 0
                    ) {

                        db.deletePatient(id);


                        response = makeHttpResponse(
                            "200 OK",
                            "Patient deleted\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid patient ID\n"
                        );
                    }
                }


                // ----------------------
                // USER REST API
                // ----------------------

                // GET /users
                else if (
                    method == "GET" &&
                    path == "/users"
                ) {

                    auto users = db.getUsers();

                    std::string result;


                    for (auto const& user : users) {

                        result +=
                            std::to_string(user.get_id()) +
                            " " +
                            user.get_username() +
                            " " +
                            user.get_role() +
                            "\n";
                    }


                    if (users.empty()) {
                        result = "No users\n";
                    }


                    response = makeHttpResponse(
                        "200 OK",
                        result
                    );
                }


                // POST /users
                // Body: 1 kevin admin
                else if (
                    method == "POST" &&
                    path == "/users"
                ) {

                    std::istringstream data{body};

                    int id;
                    std::string username;
                    std::string role;


                    if (
                        data >> id >> username >> role &&
                        id > 0 &&
                        !username.empty() &&
                        username.size() <= 50 &&
                        !role.empty() &&
                        role.size() <= 30
                    ) {

                        db.addUser(
                            id,
                            username,
                            role
                        );


                        response = makeHttpResponse(
                            "201 Created",
                            "User added\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid user data\n"
                        );
                    }
                }


                // PUT /users
                // Body: 1 kevin doctor
                else if (
                    method == "PUT" &&
                    path == "/users"
                ) {

                    std::istringstream data{body};

                    int id;
                    std::string username;
                    std::string role;


                    if (
                        data >> id >> username >> role &&
                        id > 0 &&
                        !username.empty() &&
                        username.size() <= 50 &&
                        !role.empty() &&
                        role.size() <= 30
                    ) {

                        db.updateUser(
                            id,
                            username,
                            role
                        );


                        response = makeHttpResponse(
                            "200 OK",
                            "User updated\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid user data\n"
                        );
                    }
                }


                // DELETE /users
                // Body: 1
                else if (
                    method == "DELETE" &&
                    path == "/users"
                ) {

                    std::istringstream data{body};

                    int id;


                    if (
                        data >> id &&
                        id > 0
                    ) {

                        db.deleteUser(id);


                        response = makeHttpResponse(
                            "200 OK",
                            "User deleted\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid user ID\n"
                        );
                    }
                }


                // ----------------------
                // STUDY REST API
                // ----------------------

                // GET /studies
                else if (
                    method == "GET" &&
                    path == "/studies"
                ) {

                    auto studies = db.getStudies();

                    std::string result;


                    for (auto const& study : studies) {

                        result +=
                            std::to_string(study.get_id()) +
                            " " +
                            std::to_string(study.get_patient_id()) +
                            " " +
                            study.get_description() +
                            "\n";
                    }


                    if (studies.empty()) {
                        result = "No studies\n";
                    }


                    response = makeHttpResponse(
                        "200 OK",
                        result
                    );
                }


                // POST /studies
                // Body: 10 1 Brain_MRI
                else if (
                    method == "POST" &&
                    path == "/studies"
                ) {

                    std::istringstream data{body};

                    int id;
                    int patient_id;
                    std::string description;


                    if (
                        data >> id >> patient_id >> description &&
                        id > 0 &&
                        patient_id > 0 &&
                        !description.empty() &&
                        description.size() <= 100
                    ) {

                        db.addStudy(
                            id,
                            patient_id,
                            description
                        );


                        response = makeHttpResponse(
                            "201 Created",
                            "Study added\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid study data\n"
                        );
                    }
                }


                // PUT /studies
                // Body: 10 1 Updated_Brain_MRI
                else if (
                    method == "PUT" &&
                    path == "/studies"
                ) {

                    std::istringstream data{body};

                    int id;
                    int patient_id;
                    std::string description;


                    if (
                        data >> id >> patient_id >> description &&
                        id > 0 &&
                        patient_id > 0 &&
                        !description.empty() &&
                        description.size() <= 100
                    ) {

                        db.updateStudy(
                            id,
                            patient_id,
                            description
                        );


                        response = makeHttpResponse(
                            "200 OK",
                            "Study updated\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid study data\n"
                        );
                    }
                }


                // DELETE /studies
                // Body: 10
                else if (
                    method == "DELETE" &&
                    path == "/studies"
                ) {

                    std::istringstream data{body};

                    int id;


                    if (
                        data >> id &&
                        id > 0
                    ) {

                        db.deleteStudy(id);


                        response = makeHttpResponse(
                            "200 OK",
                            "Study deleted\n"
                        );
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid study ID\n"
                        );
                    }
                }


                // ----------------------
                // ANALYSIS REST API
                // ----------------------

                // GET /analysis
                else if (
                    method == "GET" &&
                    path == "/analysis"
                ) {

                    // Run the Python ML script
                    int python_result =
                        std::system(
                            ".venv/bin/python "
                            "python/image_analysis.py"
                        );


                    // Check if Python failed
                    if (python_result != 0) {

                        response = makeHttpResponse(
                            "500 Internal Server Error",
                            "Analysis failed\n"
                        );
                    }

                    else {

                        // Open the result created by Python
                        std::ifstream file{
                            "data/analysis_result.txt"
                        };

                        std::string analysis;
                        std::string line;


                        // Read every line from the result file
                        while (std::getline(file, line)) {

                            analysis += line + "\n";
                        }


                        // Send analysis back to client
                        response = makeHttpResponse(
                            "200 OK",
                            analysis
                        );
                    }
                }


                // Unknown endpoint
                else {

                    response = makeHttpResponse(
                        "404 Not Found",
                        "Endpoint not found\n"
                    );
                }
            }


                catch (std::exception const& error) {

                    response = makeHttpResponse(
                        "400 Bad Request",
                        "ERROR: " +
                        std::string{error.what()} +
                        "\n"
                    );
                }
            }
        }


        // Send response back to this client
        send(
            client_socket,
            response.c_str(),
            response.size(),
            0
        );
    }


    // This thread is done with this client
    close(client_socket);
}