#include <gtest/gtest.h>
#include "sturdy_guide/hello.h"

TEST(HelloTest, ReturnsCorrectString) {
    EXPECT_EQ(sayHello(), "Hello, World!");
}

TEST(HelloTest, ReturnsNonEmptyString) {
    EXPECT_FALSE(sayHello().empty());
}

TEST(HelloTest, ContainsHello) {
    EXPECT_TRUE(sayHello().find("Hello") != std::string::npos);
}
