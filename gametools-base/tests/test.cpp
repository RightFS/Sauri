//
// Created by Right on 25/3/25 星期二 13:57.
//
#include <gtest/gtest.h>
// 测试夹具
class DemoTest : public ::testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
};


TEST_F(DemoTest, test1) {
    EXPECT_EQ(1, 1);
}
