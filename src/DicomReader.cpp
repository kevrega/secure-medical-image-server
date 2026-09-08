#include "DicomReader.h"

#include <dcmtk/dcmdata/dctk.h>

#include <iostream>
#include <string>


void DicomReader::readFile(std::string const& filename) {

    // Represents the whole DICOM file
    DcmFileFormat file;

    // Try to load the DICOM file
    OFCondition status = file.loadFile(filename.c_str());

    // Check if loading failed
    if (!status.good()) {
        std::cout << "Could not open DICOM file\n";
        return;
    }

    // Get the actual DICOM dataset
    DcmDataset* dataset = file.getDataset();

    OFString patient_id;
    OFString patient_name;
    OFString modality;
    OFString study_description;

    dataset->findAndGetOFString(
        DCM_PatientID,
        patient_id
    );

    dataset->findAndGetOFString(
        DCM_PatientName,
        patient_name
    );

    dataset->findAndGetOFString(
        DCM_Modality,
        modality
    );

    dataset->findAndGetOFString(
        DCM_StudyDescription,
        study_description
    );


    std::cout << "Patient ID: "
              << patient_id << '\n';

    std::cout << "Patient Name: "
              << patient_name << '\n';

    std::cout << "Modality: "
              << modality << '\n';

    std::cout << "Study Description: "
              << study_description << '\n';
}

// DICOM format: (TAG) VR [VALUE]
// TAG = what the field is
// VR = data type
// VALUE = stored value
// Example: (0010,0020) LO [12345] = Patient ID 12345