#ifndef DICOMREADER_H
#define DICOMREADER_H

#include <string>

class DicomReader {
public:
    void readFile(std::string const& filename);
};

#endif