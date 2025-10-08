#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "Utils.h"

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
