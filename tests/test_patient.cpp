#include <gtest/gtest.h>
#include "Patient.h"

TEST(PatientTest, StoresPatientInformation)
{
    Patient patient{1, "John_Doe", 45};

    EXPECT_EQ(patient.get_id(), 1);
    EXPECT_EQ(patient.get_name(), "John_Doe");
    EXPECT_EQ(patient.get_age(), 45);
}