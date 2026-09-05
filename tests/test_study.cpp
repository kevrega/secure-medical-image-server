#include <gtest/gtest.h>
#include "Study.h"

TEST(StudyTest, StoresStudyInformation)
{
    Study study{10, 1, "Brain_MRI"};

    EXPECT_EQ(study.get_id(), 10);
    EXPECT_EQ(study.get_patient_id(), 1);
    EXPECT_EQ(study.get_description(), "Brain_MRI");
}