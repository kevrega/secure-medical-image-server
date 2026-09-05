#include <gtest/gtest.h>
#include "User.h"

TEST(UserTest, StoresUserInformation)
{
    User user{1, "kevin", "admin"};

    EXPECT_EQ(user.get_id(), 1);
    EXPECT_EQ(user.get_username(), "kevin");
    EXPECT_EQ(user.get_role(), "admin");
}