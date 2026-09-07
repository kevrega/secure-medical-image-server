#include <iostream>

#include "TcpServer.h"

int main() {

    // Use this for server end

    TcpServer server{1337};
    server.start();

    return 0;
}

/*
    curl -X GET http://localhost:1337/patients
    curl sends an HTTP GET request to server at localhost on port 1337
    and asks for the /patients path
*/
