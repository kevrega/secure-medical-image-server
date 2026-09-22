#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sstream>
#include <sys/socket.h>

namespace {
constexpr std::size_t maxBody = 20 * 1024 * 1024;
constexpr std::size_t maxHeaders = 16 * 1024;

void receiveMore(int socket, std::string& message) {
    char buffer[16384];
    ssize_t count;
    do { count = recv(socket, buffer, sizeof(buffer), 0); } while (count < 0 && errno == EINTR);
    if (count <= 0) throw HttpError{"400 Bad Request", "Incomplete or timed out request"};
    message.append(buffer, static_cast<std::size_t>(count));
}
}

HttpRequest receiveHttpRequest(int socket, std::string initial) {
    std::size_t end;
    while ((end = initial.find("\r\n\r\n")) == std::string::npos) {
        if (initial.size() > maxHeaders) throw HttpError{"431 Request Header Fields Too Large", "Headers too large"};
        receiveMore(socket, initial);
    }
    if (end > maxHeaders) throw HttpError{"431 Request Header Fields Too Large", "Headers too large"};
    HttpRequest request;
    std::istringstream lines{initial.substr(0, end)};
    std::string line;
    std::getline(lines, line);
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == std::string::npos) throw HttpError{"400 Bad Request", "Invalid header"};
        auto name = line.substr(0, colon);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return std::tolower(ch); });
        auto value = line.substr(colon + 1);
        auto first = value.find_first_not_of(" \t");
        value = first == std::string::npos ? "" : value.substr(first, value.find_last_not_of(" \t") - first + 1);
        if (!request.headers.emplace(name, value).second)
            throw HttpError{"400 Bad Request", "Duplicate header"};
    }
    if (request.headers.count("transfer-encoding"))
        throw HttpError{"400 Bad Request", "Use Content-Length; chunked uploads are not supported"};
    std::size_t length = 0;
    if (request.headers.count("content-length")) {
        auto const& value = request.headers.at("content-length");
        if (value.empty() || value.size() > 10 || !std::all_of(value.begin(), value.end(), [](unsigned char ch) { return std::isdigit(ch); }))
            throw HttpError{"400 Bad Request", "Invalid Content-Length"};
        length = std::stoull(value);
    }
    if (length > maxBody) throw HttpError{"413 Payload Too Large", "Maximum upload size is 20 MiB"};
    if (request.headers.count("expect"))
        throw HttpError{"417 Expectation Failed", "Send the request without Expect"};
    auto total = end + 4 + length;
    while (initial.size() < total) receiveMore(socket, initial);
    initial.resize(total);
    request.message = std::move(initial);
    return request;
}

void sendAll(int socket, std::string const& response) {
    std::size_t sent = 0;
    while (sent < response.size()) {
        auto count = send(socket, response.data() + sent, response.size() - sent, MSG_NOSIGNAL);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return;
        sent += static_cast<std::size_t>(count);
    }
}

std::string httpResponse(std::string const& status, std::string const& body, std::string const& type) {
    return "HTTP/1.1 " + status + "\r\nContent-Type: " + type +
        "\r\nContent-Length: " + std::to_string(body.size()) +
        "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n" + body;
}

std::string jsonString(std::string const& value) {
    const char* hex = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') { result += '\\'; result += ch; }
        else if (ch < 32) { result += "\\u00"; result += hex[ch >> 4]; result += hex[ch & 15]; }
        else result += ch;
    }
    return result + '"';
}
