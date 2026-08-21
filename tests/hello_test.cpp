#include <gtest/gtest.h>
int hello();
TEST(HelloTest, Basic) {
    EXPECT_EQ(hello(), 42);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
