#include "DicomReader.h"

#include <dcmtk/dcmdata/dctk.h>

#include <iostream>
#include <stdexcept>
#include <string>


DicomInfo DicomReader::readFile(std::string const& filename) {

    // Represents the whole DICOM file
    DcmFileFormat file;

    // Try to load the DICOM file
    OFCondition status = file.loadFile(filename.c_str());

    // Check if loading failed
    if (!status.good()) {
        throw std::runtime_error{"Could not open DICOM file"};
    }

    // Get the real DICOM dataset
    DcmDataset* dataset = file.getDataset();

    OFString patient_id;
    OFString patient_name;
    OFString patient_age;
    OFString study_id;
    OFString modality;
    OFString study_description;

    dataset->findAndGetOFString(DCM_PatientID, patient_id);
    dataset->findAndGetOFString(DCM_PatientName, patient_name);
    dataset->findAndGetOFString(DCM_PatientAge, patient_age);
    dataset->findAndGetOFString(DCM_StudyID, study_id);
    dataset->findAndGetOFString(DCM_Modality, modality);
    dataset->findAndGetOFString(DCM_StudyDescription, study_description);

    int patient_id_number;
    int patient_age_number = 0;
    int study_id_number;

    try {
        // Our current database uses INTEGER patient IDs
        patient_id_number = std::stoi(std::string{patient_id.c_str()});

        // DICOM PatientAge normally looks like 025Y
        // stoi reads 025 and ignores Y
        if (!patient_age.empty()) {
            patient_age_number = std::stoi(std::string{patient_age.c_str()});
        }

        // Our current database uses INTEGER study IDs
        study_id_number = std::stoi(std::string{study_id.c_str()});
    }

    catch (...) {
        throw std::runtime_error{"DICOM Patient ID and Study ID must be numeric"};
    }

    std::cout << "Patient ID: " << patient_id << '\n';
    std::cout << "Patient Name: " << patient_name << '\n';
    std::cout << "Patient Age: " << patient_age << '\n';
    std::cout << "Study ID: " << study_id << '\n';
    std::cout << "Modality: " << modality << '\n';
    std::cout << "Study Description: " << study_description << '\n';

    return DicomInfo{
        patient_id_number,
        std::string{patient_name.c_str()},
        patient_age_number,
        study_id_number,
        std::string{modality.c_str()},
        std::string{study_description.c_str()}
    };
}


// DICOM format: (TAG) VR [VALUE]
// TAG = what the field is
// VR = data type
// VALUE = stored value
// Example: (0010,0020) LO [12345] = Patient ID 12345