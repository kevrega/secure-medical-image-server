#ifndef DICOMREADER_H
#define DICOMREADER_H

#include <string>


struct DicomInfo {

    int patient_id;
    std::string patient_name;
    int patient_age;

    int study_id;
    std::string modality;
    std::string study_description;
};


class DicomReader {

public:

    DicomInfo readFile(std::string const& filename);
};


#endif