#ifndef IMAGE_SERVICE_H
#define IMAGE_SERVICE_H

#include "Database.h"
#include <map>
#include <string>

bool handleImageRequest(std::string const& method, std::string const& path,
                        std::map<std::string, std::string> const& headers,
                        std::string const& body, Database& db, std::string& response);

#endif
