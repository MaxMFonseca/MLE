#include "Utils.h"

TEST(UtilStrTo, ParsesValidInt) {
    auto r = mle::strTo<int>("12345");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 12345);
}

TEST(UtilStrTo, ParsesValidNegativeInt) {
    auto r = mle::strTo<int>("-12345");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, -12345);
}

TEST(UtilStrTo, FailsOnInvalidInt) {
    auto r = mle::strTo<int>("12a45");
    EXPECT_FALSE(r.has_value());
}

TEST(UtilStrTo, ParsesValidUnsignedInt) {
    auto r = mle::strTo<unsigned int>("42");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42U);
}

TEST(UtilStrTo, FailsOnNegativeUnsignedInt) {
    auto r = mle::strTo<unsigned int>("-1");
    EXPECT_FALSE(r.has_value());
}

TEST(UtilStrTo, ParsesValidFloat) {
    auto r = mle::strTo<float>("3.1415");
    ASSERT_TRUE(r.has_value());
    EXPECT_FLOAT_EQ(*r, 3.1415F);
}

TEST(UtilStrTo, ParsesValidDoubleScientific) {
    auto r = mle::strTo<double>("1.23e4");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(*r, 12300.0);
}

TEST(UtilStrTo, FailsOnEmptyString) {
    auto r = mle::strTo<int>("");
    EXPECT_FALSE(r.has_value());
}

TEST(UtilStrTo, FailsOnWhitespaceOnly) {
    auto r = mle::strTo<int>("   ");
    EXPECT_FALSE(r.has_value());
}

TEST(UtilStrTo, ParsesValidLong) {
    auto r = mle::strTo<i64>("123456789");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 123456789L);
}

TEST(UtilStrTo, FailsOnOverflowInt) {
    auto r = mle::strTo<int>("99999999999999999999");
    EXPECT_FALSE(r.has_value());
}
