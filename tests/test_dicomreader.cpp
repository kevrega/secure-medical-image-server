#include <gtest/gtest.h>

#include "DicomReader.h"

#include <string>


TEST(DicomReaderTest, ReadsMetadata) {

    DicomReader reader;

    // Capture what DicomReader prints
    testing::internal::CaptureStdout();

    reader.readFile("../data/test.dcm"); // Run the function on DICOM file

    std::string output =
        testing::internal::GetCapturedStdout(); 
        // Stop capture terminal output and store inside string

    EXPECT_NE(output.find("Patient ID: 12345"), std::string::npos); 
    // Expect not equal, i.e. we don't expect the result of find() to be equal "not found"
    EXPECT_NE(output.find("Patient Name: Kevin^Test"), std::string::npos);
    EXPECT_NE(output.find("Modality: MR"), std::string::npos);
    EXPECT_NE(output.find("Study Description: Brain MRI"), std::string::npos);
}