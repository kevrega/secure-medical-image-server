#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <map>
#include <stdexcept>
#include <string>

struct HttpError : std::runtime_error {
    std::string status;
    HttpError(std::string status, std::string message)
        : std::runtime_error{message}, status{status} {}
};

struct HttpRequest {
    std::string message;
    std::map<std::string, std::string> headers;
};

HttpRequest receiveHttpRequest(int socket, std::string initial);
void sendAll(int socket, std::string const& response);
std::string httpResponse(std::string const& status, std::string const& body,
                         std::string const& type = "application/json");
std::string jsonString(std::string const& value);

#endif
