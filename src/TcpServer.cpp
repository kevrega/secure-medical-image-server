#include "TcpServer.h"
#include "Database.h"
#include "DicomReader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib> // system()
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


// Create a simple HTTP response
std::string makeHttpResponse(
    std::string const& status,
    std::string const& body,
    std::string const& content_type = "text/plain"
) {
    return
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
}


// Convert text to lowercase
std::string toLower(std::string text) {

    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );

    return text;
}


// Remove spaces from the beginning and end of a string
std::string trim(std::string text) {

    while (
        !text.empty() &&
        std::isspace(static_cast<unsigned char>(text.front()))
    ) {
        text.erase(text.begin());
    }

    while (
        !text.empty() &&
        std::isspace(static_cast<unsigned char>(text.back()))
    ) {
        text.pop_back();
    }

    return text;
}


// Find one HTTP header
// HTTP header names are case-insensitive
std::string getHeaderValue(
    std::string const& headers,
    std::string const& wanted_header
) {

    std::istringstream input{headers};

    std::string line;

    // Skip the first request line
    std::getline(input, line);


    while (std::getline(input, line)) {

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }


        std::size_t colon_position = line.find(':');

        if (colon_position == std::string::npos) {
            continue;
        }


        std::string header_name =
            trim(line.substr(0, colon_position));


        if (
            toLower(header_name) ==
            toLower(wanted_header)
        ) {
            return trim(
                line.substr(colon_position + 1)
            );
        }
    }


    return "";
}


// Send the complete response
// send() is allowed to send fewer bytes than requested
void sendAll(
    int client_socket,
    std::string const& response
) {

    std::size_t total_sent = 0;


    while (total_sent < response.size()) {

        ssize_t sent = send(
            client_socket,
            response.data() + total_sent,
            response.size() - total_sent,
            0
        );


        if (sent <= 0) {
            break;
        }


        total_sent += static_cast<std::size_t>(sent);
    }
}


// Read the Python analysis result into one string
std::string readAnalysisResult() {

    std::ifstream file{
        "data/analysis_result.txt"
    };


    if (!file) {
        throw std::runtime_error{
            "Could not read analysis result"
        };
    }


    std::ostringstream result;
    result << file.rdbuf();

    return result.str();
}


// Only allow a simple uploaded filename
std::string safeFilename(std::string filename) {

    if (
        filename.empty() ||
        filename.find("..") != std::string::npos ||
        filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos
    ) {
        throw std::runtime_error{
            "Invalid image filename"
        };
    }


    for (char& character : filename) {

        unsigned char value =
            static_cast<unsigned char>(character);


        if (
            !std::isalnum(value) &&
            character != '.' &&
            character != '_' &&
            character != '-'
        ) {
            character = '_';
        }
    }


    return filename;
}


// Check if Python knows how to read this image type
bool supportedImage(std::string const& filename) {

    std::string extension =
        toLower(
            std::filesystem::path{filename}
                .extension()
                .string()
        );


    return
        extension == ".dcm" ||
        extension == ".png" ||
        extension == ".jpg" ||
        extension == ".jpeg";
}


// Run the Python image analysis
std::string runImageAnalysis(
    std::string const& image_path = ""
) {

    std::string command =
        ".venv/bin/python "
        "python/image_analysis.py";


    if (!image_path.empty()) {
        command += " " + image_path;
    }


    int python_result =
        std::system(command.c_str());


    if (python_result != 0) {
        throw std::runtime_error{
            "Analysis failed"
        };
    }


    return readAnalysisResult();
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


    // Create the tables once before client threads start.
    // Each client will still open its own database connection.
    {
        Database db{"data/medical.db"};
        db.createTables();
    }


    std::cout << "Listening on port " << port << '\n';


    while (true) {

        // WAIT here until a client connects
        // accept() creates a new client socket for that connection
        int client_socket =
            accept(server_socket, nullptr, nullptr);


        if (client_socket == -1) {

            // 0 = Success, -1 = Failure
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


    // Buffer where received client data will be stored
    char buffer[8192]{};


    // Receive data from this client
    ssize_t bytes_received = recv(
        client_socket,         // Receive from specific client
        buffer,                // Store data here
        sizeof(buffer),        // Max data to receive
        0
    );


    if (bytes_received > 0) {

        // Use the exact number of bytes received.
        // This also lets std::string store binary image data.
        std::string message{
            buffer,
            static_cast<std::size_t>(bytes_received)
        };


        // Find the end of the HTTP headers first
        std::size_t body_position =
            message.find("\r\n\r\n");


        while (body_position == std::string::npos) {

            bytes_received = recv(
                client_socket,
                buffer,
                sizeof(buffer),
                0
            );


            if (bytes_received <= 0) {
                break;
            }


            message.append(
                buffer,
                static_cast<std::size_t>(bytes_received)
            );


            // Do not allow an extremely large HTTP header
            if (message.size() > 64 * 1024) {
                break;
            }


            body_position =
                message.find("\r\n\r\n");
        }


        std::string response;


        // ----------------------
        // HTTP / REST REQUESTS
        // ----------------------

        bool is_http =
            message.find("GET ") == 0 ||
            message.find("POST ") == 0 ||
            message.find("PUT ") == 0 ||
            message.find("DELETE ") == 0;


        if (
            is_http &&
            body_position != std::string::npos
        ) {

            // First line looks like:
            // GET /patients HTTP/1.1
            std::istringstream request{message};

            std::string method;
            std::string path;
            std::string http_version;

            request >> method >> path >> http_version;


            // Keep only the HTTP header section
            std::string headers =
                message.substr(0, body_position);


            // Find HTTP body (everything after empty line)
            std::string body;


            // Find how many body bytes the client says it sent
            std::string content_length_text =
                getHeaderValue(
                    headers,
                    "Content-Length"
                );

            std::size_t content_length = 0;


            if (!content_length_text.empty()) {

                try {
                    content_length =
                        std::stoull(content_length_text);
                }

                catch (...) {
                    response = makeHttpResponse(
                        "400 Bad Request",
                        "Invalid Content-Length\n"
                    );

                    sendAll(client_socket, response);
                    close(client_socket);
                    return;
                }
            }


            // Normal REST request bodies stay small.
            // Image uploads are allowed to be larger.
            std::size_t max_body_size =
                path == "/analyze-image"
                    ? 15 * 1024 * 1024
                    : 512;


            // Reject request if the body is too large
            if (content_length > max_body_size) {

                response = makeHttpResponse(
                    "413 Payload Too Large",
                    "Request body is too large\n"
                );

                sendAll(
                    client_socket,
                    response
                );

                close(client_socket);
                return;
            }


            // If "\r\n\r\n" was found,
            // there is a body after the HTTP headers
            std::size_t body_start =
                body_position + 4;


            // A file can be much larger than one recv() call.
            // Keep receiving until the complete HTTP body is here.
            while (
                message.size() - body_start <
                content_length
            ) {

                bytes_received = recv(
                    client_socket,
                    buffer,
                    sizeof(buffer),
                    0
                );


                if (bytes_received <= 0) {
                    break;
                }


                message.append(
                    buffer,
                    static_cast<std::size_t>(bytes_received)
                );
            }


            if (
                message.size() - body_start <
                content_length
            ) {
                response = makeHttpResponse(
                    "400 Bad Request",
                    "Incomplete HTTP body\n"
                );

                sendAll(client_socket, response);
                close(client_socket);
                return;
            }


            // Take everything after "\r\n\r\n"
            // and store it as the request body
            body = message.substr(
                body_start,
                content_length
            );


            // Do not print binary image bytes to the terminal
            std::cout
                << "Received: "
                << method
                << " "
                << path
                << "\n";


            // ----------------------
            // BASIC SECURITY
            // ----------------------

            // Get the expected API key from an environment variable
            const char* expected_key =
                std::getenv("MEDICAL_API_KEY");


            // Find the API key sent by the client
            std::string api_key;

            std::string key_header = "X-API-Key: ";

            std::size_t key_position = headers.find(key_header);

            // Vite may send the header name in lowercase
            if (key_position == std::string::npos) {
                key_header = "x-api-key: ";
                key_position = headers.find(key_header);
            }

            if (key_position != std::string::npos) {

                // Start reading after the API key header
                std::size_t key_start = key_position + key_header.size();

                // Find the end of this HTTP header
                std::size_t key_end = headers.find("\r\n", key_start);

                // Extract only the API key
                api_key =
                    headers.substr(
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
                // STUDY IMAGE REST API
                // ----------------------

                // GET /images?study_id=10
                // Returns the images already connected to one study
                else if (
                    method == "GET" &&
                    path.find("/images?study_id=") == 0
                ) {

                    std::string study_id_text =
                        path.substr(
                            std::string{"/images?study_id="}.size()
                        );


                    int study_id;


                    try {
                        study_id =
                            std::stoi(study_id_text);
                    }

                    catch (...) {
                        throw std::runtime_error{
                            "Invalid study ID"
                        };
                    }


                    if (study_id <= 0) {
                        throw std::runtime_error{
                            "Invalid study ID"
                        };
                    }


                    auto images =
                        db.getImages(study_id);

                    std::string result;


                    for (auto const& image : images) {

                        result +=
                            std::to_string(image.id) +
                            " " +
                            image.filename +
                            "\n";
                    }


                    if (images.empty()) {
                        result = "No images\n";
                    }


                    response = makeHttpResponse(
                        "200 OK",
                        result
                    );
                }


                // DELETE /images
                // Body: study_id image_id
                else if (
                    method == "DELETE" &&
                    path == "/images"
                ) {

                    std::istringstream data{body};

                    int study_id;
                    int image_id;


                    if (
                        !(data >> study_id >> image_id) ||
                        study_id <= 0 ||
                        image_id <= 0
                    ) {
                        throw std::runtime_error{
                            "Invalid image selection"
                        };
                    }


                    auto images =
                        db.getImages(study_id);

                    bool image_found = false;
                    std::string image_path;


                    for (auto const& image : images) {

                        if (image.id == image_id) {
                            image_found = true;
                            image_path =
                                image.storage_path;
                            break;
                        }
                    }


                    if (!image_found) {
                        throw std::runtime_error{
                            "Image does not belong to the selected study"
                        };
                    }


                    // Remove the file from disk before removing its database row
                    if (
                        std::filesystem::exists(
                            image_path
                        )
                    ) {

                        if (
                            !std::filesystem::remove(
                                image_path
                            )
                        ) {
                            throw std::runtime_error{
                                "Could not delete the stored image file"
                            };
                        }
                    }


                    db.deleteImage(
                        image_id,
                        study_id
                    );


                    response = makeHttpResponse(
                        "200 OK",
                        "Image deleted\n"
                    );
                }


                // ----------------------
                // DICOM IMPORT
                // ----------------------

                // POST /dicom
                // Body: scan.dcm
                else if (
                    method == "POST" &&
                    path == "/dicom"
                ) {

                    std::istringstream data{body};

                    std::string filename;

                    // Read the filename from the HTTP body
                    //
                    // Also reject paths such as:
                    // ../secret.txt
                    // folder/scan.dcm
                    //
                    // We only want a simple filename located inside data/

                    if (
                        data >> filename &&
                        filename.find("..") == std::string::npos &&
                        filename.find('/') == std::string::npos &&
                        filename.find('\\') == std::string::npos
                    ) {

                        // DICOM files are read only from data/
                        std::string dicom_path =
                            "data/" + filename;


                        DicomReader reader;

                        DicomInfo info =
                            reader.readFile(
                                dicom_path
                            );


                        // ----------------------
                        // ADD PATIENT
                        // ----------------------

                        bool patient_exists = false;

                        auto patients =
                            db.getPatients();


                        for (auto const& patient : patients) {

                            if (
                                patient.get_id() ==
                                info.patient_id
                            ) {

                                patient_exists = true;
                                break;
                            }
                        }


                        // Only add patient if this patient
                        // is not already in the database
                        if (!patient_exists) {

                            db.addPatient(
                                info.patient_id,
                                info.patient_name,
                                info.patient_age
                            );
                        }


                        // ----------------------
                        // ADD STUDY
                        // ----------------------

                        bool study_exists = false;

                        auto studies =
                            db.getStudies();


                        for (auto const& study : studies) {

                            if (
                                study.get_id() ==
                                info.study_id
                            ) {

                                study_exists = true;
                                break;
                            }
                        }


                        // Only add study if this study
                        // is not already in the database
                        if (!study_exists) {

                            db.addStudy(
                                info.study_id,
                                info.patient_id,
                                info.study_description
                            );


                            response = makeHttpResponse(
                                "201 Created",
                                "DICOM patient and study imported\n"
                            );
                        }

                        else {

                            response = makeHttpResponse(
                                "200 OK",
                                "DICOM study already exists\n"
                            );
                        }
                    }

                    else {

                        response = makeHttpResponse(
                            "400 Bad Request",
                            "Invalid DICOM filename\n"
                        );
                    }
                }


                // ----------------------
                // EXISTING IMAGE ANALYSIS
                // ----------------------

                // POST /analyze-existing-image
                // Body: study_id image_id
                else if (
                    method == "POST" &&
                    path == "/analyze-existing-image"
                ) {

                    std::istringstream data{body};

                    int study_id;
                    int image_id;


                    if (
                        !(data >> study_id >> image_id) ||
                        study_id <= 0 ||
                        image_id <= 0
                    ) {
                        throw std::runtime_error{
                            "Invalid image selection"
                        };
                    }


                    auto images =
                        db.getImages(study_id);

                    bool image_found = false;
                    std::string image_path;


                    for (auto const& image : images) {

                        if (image.id == image_id) {
                            image_found = true;
                            image_path =
                                image.storage_path;
                            break;
                        }
                    }


                    if (!image_found) {
                        throw std::runtime_error{
                            "Image does not belong to the selected study"
                        };
                    }


                    if (
                        !std::filesystem::exists(
                            image_path
                        )
                    ) {
                        throw std::runtime_error{
                            "The stored image file could not be found"
                        };
                    }


                    // Run the same analysis on an image that was uploaded earlier
                    std::string analysis =
                        runImageAnalysis(
                            image_path
                        );


                    response = makeHttpResponse(
                        "200 OK",
                        analysis,
                        "application/json"
                    );
                }


                // ----------------------
                // IMAGE UPLOAD + ANALYSIS
                // ----------------------

                // POST /analyze-image
                // Body: raw DICOM / PNG / JPG bytes
                // X-Study-ID tells us which study this image belongs to
                else if (
                    method == "POST" &&
                    path == "/analyze-image"
                ) {

                    // Browser sends the original filename in this header
                    std::string filename =
                        getHeaderValue(
                            headers,
                            "X-Filename"
                        );


                    filename = safeFilename(filename);


                    if (!supportedImage(filename)) {
                        throw std::runtime_error{
                            "Supported files: DICOM, PNG, JPG and JPEG"
                        };
                    }


                    if (body.empty()) {
                        throw std::runtime_error{
                            "No image was uploaded"
                        };
                    }


                    // Browser also sends the study that should own this image
                    std::string study_id_text =
                        getHeaderValue(
                            headers,
                            "X-Study-ID"
                        );


                    if (study_id_text.empty()) {
                        throw std::runtime_error{
                            "Study ID is missing"
                        };
                    }


                    int study_id;


                    try {
                        study_id = std::stoi(study_id_text);
                    }

                    catch (...) {
                        throw std::runtime_error{
                            "Invalid study ID"
                        };
                    }


                    // Check that the selected study exists before saving the file
                    bool study_exists = false;

                    auto studies =
                        db.getStudies();


                    for (auto const& study : studies) {

                        if (study.get_id() == study_id) {
                            study_exists = true;
                            break;
                        }
                    }


                    if (!study_exists) {
                        throw std::runtime_error{
                            "Study does not exist"
                        };
                    }


                    // Do not overwrite an image already stored for this study
                    auto existing_images =
                        db.getImages(study_id);


                    for (auto const& image : existing_images) {

                        if (image.filename == filename) {
                            throw std::runtime_error{
                                "This study already contains an image named " +
                                filename +
                                "."
                            };
                        }
                    }


                    // Keep each study's uploaded images in its own folder
                    std::string upload_directory =
                        "data/uploads/" +
                        std::to_string(study_id);


                    std::filesystem::create_directories(
                        upload_directory
                    );


                    std::string upload_path =
                        upload_directory +
                        "/" +
                        filename;


                    // Save the uploaded binary data exactly as it arrived
                    std::ofstream image_file{
                        upload_path,
                        std::ios::binary
                    };


                    if (!image_file) {
                        throw std::runtime_error{
                            "Could not save uploaded image"
                        };
                    }


                    image_file.write(
                        body.data(),
                        static_cast<std::streamsize>(body.size())
                    );

                    image_file.close();


                    // Run Python on the image the user just uploaded
                    std::string analysis =
                        runImageAnalysis(upload_path);


                    // Store the link between this image and its study
                    db.addImage(
                        study_id,
                        filename,
                        upload_path
                    );


                    // Python writes JSON containing the result and images
                    response = makeHttpResponse(
                        "200 OK",
                        analysis,
                        "application/json"
                    );
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
                    // No file path means Python uses its demo CT image
                    std::string analysis =
                        runImageAnalysis();


                    // Send analysis back to client
                    response = makeHttpResponse(
                        "200 OK",
                        analysis,
                        "application/json"
                    );
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
        // Large analysis responses may need more than one send() call
        sendAll(
            client_socket,
            response
        );
    }


    // This thread is done with this client
    close(client_socket);
}