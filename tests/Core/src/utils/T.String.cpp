#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "Utils.h"
#include "mle/utils/String.h"

TEST(StringSplitTest, BasicSplit) {
    auto result = mle::split("a,b,c", ',', false);
    std::vector<std::string_view> expected{"a", "b", "c"};
    EXPECT_EQ(result, expected);
}

TEST(StringSplitTest, EmptyInput) {
    auto result = mle::split("", ',', false);
    EXPECT_TRUE(result.empty());
}

TEST(StringSplitTest, ConsecutiveDelimitersKeepEmpty) {
    auto result = mle::split("a,,b,", ',', true);
    std::vector<std::string_view> expected{"a", "", "b", ""};
    EXPECT_EQ(result, expected);
}

TEST(StringSplitTest, ConsecutiveDelimitersSkipEmpty) {
    auto result = mle::split("a,,b,", ',', false);
    std::vector<std::string_view> expected{"a", "b"};
    EXPECT_EQ(result, expected);
}

TEST(StringSplitTest, LeadingTrailingDelimiters) {
    auto result = mle::split(",a,b,", ',', true);
    std::vector<std::string_view> expected{"", "a", "b", ""};
    EXPECT_EQ(result, expected);
}

TEST(StringSplitTest, LoremIpsumSplit) {
    auto result = mle::split("Lorem,ipsum,,dolor,sit,amet,,consectetur", ',', true);
    std::vector<std::string_view> expected{"Lorem", "ipsum", "", "dolor", "sit", "amet", "", "consectetur"};
    EXPECT_EQ(result, expected);

    auto result_skip = mle::split("Lorem,ipsum,,dolor,sit,amet,,consectetur", ',', false);
    std::vector<std::string_view> expected_skip{"Lorem", "ipsum", "dolor", "sit", "amet", "consectetur"};
    EXPECT_EQ(result_skip, expected_skip);
}

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

TEST(ToLower, BasicAscii) {
    EXPECT_EQ(mle::toLower("ABC"), "abc");
}

TEST(ToLower, MixedAlnumKeepsDigits) {
    EXPECT_EQ(mle::toLower("AbC123Z"), "abc123z");
}

TEST(ToLower, AlreadyLowerIsStable) {
    EXPECT_EQ(mle::toLower("already lower"), "already lower");
}

TEST(ToLower, NonAsciiUnchanged) {
    EXPECT_EQ(mle::toLower("ÄÖÜ ç ñ"), "ÄÖÜ ç ñ");
}

TEST(ToUpper, BasicAscii) {
    EXPECT_EQ(mle::toUpper("abc"), "ABC");
}

TEST(ToUpper, MixedAlnumKeepsDigits) {
    EXPECT_EQ(mle::toUpper("AbC123z"), "ABC123Z");
}

TEST(ToUpper, AlreadyUpperIsStable) {
    EXPECT_EQ(mle::toUpper("ALREADY UPPER"), "ALREADY UPPER");
}

TEST(ToUpper, NonAsciiUnchanged) {
    EXPECT_EQ(mle::toUpper("äöü Ç ñ"), "äöü Ç ñ");
}

TEST(ToUtf8, ASCII) {
    std::u32string s32 = U"Hello";
    EXPECT_EQ(mle::toUtf8(s32), "Hello");
}

TEST(ToUtf8, Emoji) {
    std::u32string s32 = U"Hi 🌍";
    EXPECT_EQ(mle::toUtf8(s32), std::string("Hi 🌍"));
}

TEST(ToUtf8, CJK) {
    std::u32string s32 = U"你好世界";
    EXPECT_EQ(mle::toUtf8(s32), std::string("你好世界"));
}

TEST(ToUtf8, Empty) {
    std::u32string s32;
    EXPECT_EQ(mle::toUtf8(s32), "");
}

TEST(ToUtf32, ASCII) {
    auto s32 = mle::toUtf32("Hello");
    ASSERT_EQ(s32.size(), 5U);
    EXPECT_EQ(s32, U"Hello");
}

TEST(ToUtf32, Emoji) {
    auto s32 = mle::toUtf32("Hi 🌍");
    EXPECT_EQ(s32, U"Hi 🌍");
}

TEST(ToUtf32, CJK) {
    auto s32 = mle::toUtf32("你好世界");
    EXPECT_EQ(s32, U"你好世界");
}

TEST(ToUtf32, Empty) {
    auto s32 = mle::toUtf32("");
    EXPECT_TRUE(s32.empty());
}
