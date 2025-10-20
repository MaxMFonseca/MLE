#include <gtest/gtest.h>

#include "../utils/Utils.h"
#include "mle/lua/Lua.h"
#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"

class LuaTest : public ::testing::Test {
  protected:
    Lua lua_;
    void SetUp() override { lua_.init(); }
};

TEST_F(LuaTest, CreateAndGetTable) {
    auto tbl = lua_.createTable("TestTable");
    tbl["foo"] = 42;
    auto fetched = lua_.getTable("TestTable");
    EXPECT_EQ(fetched["foo"].get<int>(), 42);
}

TEST_F(LuaTest, MergeTables) {
    auto t1 = lua_.createTable();
    t1["a"] = 1;
    auto t2 = lua_.createTable();
    t2["b"] = 2;
    lua_.mergeTables(t1, t2);
    EXPECT_EQ(t1["a"].get<int>(), 1);
    EXPECT_EQ(t1["b"].get<int>(), 2);
}

TEST_F(LuaTest, RequireModule) {
    lua_.require("mle/utils");
    auto tbl = lua_.getTable("LuaUtils");
    EXPECT_TRUE(tbl.valid());
}

TEST_F(LuaTest, ToStringWorks) {
    auto tbl = lua_.createTable();
    tbl["x"] = 123;
    std::string s = lua::toString(tbl);
    EXPECT_NE(s.find("x = 123"), std::string::npos);
}

TEST_F(LuaTest, UtilsTryGetKeyAnyKey) {
    auto tbl = lua_.createTable();
    tbl["foo"] = 7;
    tbl[1] = 8;
    auto v1 = lua::getFirstKey(tbl, "foo", 1);
    ASSERT_TRUE(v1.valid());
    EXPECT_EQ(v1.as<int>(), 8);
    auto v2 = lua::getFirstKey(tbl, "foo", 99);
    ASSERT_TRUE(v2.valid());
    EXPECT_EQ(v2.as<int>(), 7);
}

TEST_F(LuaTest, UtilsTypeConversions) {
    auto tbl = lua_.createTable();
    tbl[1] = 1.0F;
    tbl[2] = 2.0F;
    auto v = lua::as<vec2f>(tbl);
    EXPECT_FLOAT_EQ(v.x, 1.0F);
    EXPECT_FLOAT_EQ(v.y, 2.0F);
}

TEST_F(LuaTest, RequireHelloLua) {
    auto obj = lua_.require("i/HelloLua");
    ASSERT_TRUE(obj.is<sol::table>());
    auto table = obj.as<sol::table>();
    auto hello_func = table["hello"];
    ASSERT_TRUE(hello_func.valid());
    auto result = hello_func();
    EXPECT_EQ(result.get<std::string>(), "Hello from Lua!");
}

TEST_F(LuaTest, MathUtilsSum) {
    auto math_utils = lua_.require("i/MathUtils");
    ASSERT_TRUE(math_utils.is<sol::table>());
    auto sum = math_utils.as<sol::table>()["sum"];
    ASSERT_TRUE(sum.valid());
    auto result = sum(10, 32);
    EXPECT_EQ(result.get<int>(), 42);
}

TEST_F(LuaTest, MathUtilsVec2Add) {
    auto math_utils = lua_.require("i/MathUtils");
    auto vec2_add = math_utils.as<sol::table>()["vec2_add"];
    sol::table v1 = lua_.createTable();
    v1[1] = 3.0F;
    v1[2] = 4.0F;
    sol::table v2 = lua_.createTable();
    v2[1] = 1.0F;
    v2[2] = 2.0F;
    auto result = vec2_add(v1, v2);
    sol::table res_tbl = result.get<sol::table>();
    EXPECT_FLOAT_EQ(res_tbl[1].get<float>(), 4.0F);
    EXPECT_FLOAT_EQ(res_tbl[2].get<float>(), 6.0F);
}

TEST_F(LuaTest, MathUtilsIsEven) {
    auto math_utils = lua_.require("i/MathUtils");
    auto is_even = math_utils.as<sol::table>()["is_even"];
    EXPECT_TRUE(is_even(4).get<bool>());
    EXPECT_FALSE(is_even(5).get<bool>());
}

TEST_F(LuaTest, UtilsClamp) {
    auto math_clamp = lua_.getTable("math")["clamp"];
    ASSERT_TRUE(math_clamp.valid());
    EXPECT_EQ(math_clamp(5, 1, 10).get<int>(), 5);
    EXPECT_EQ(math_clamp(-5, 1, 10).get<int>(), 1);
    EXPECT_EQ(math_clamp(15, 1, 10).get<int>(), 10);
}

TEST_F(LuaTest, TableDeepCopy) {
    auto tbl = lua_.createTable();
    tbl["a"] = 1;
    sol::table sub = lua_.createTable();
    sub["b"] = 2;
    tbl["sub"] = sub;
    auto deep_copy = lua_.getTable("table")["deep_copy"];
    sol::table tbl_copy = deep_copy(tbl);
    EXPECT_EQ(tbl_copy["a"].get<int>(), 1);
    EXPECT_EQ(tbl_copy["sub"]["b"].get<int>(), 2);
    // Mutate original, ensure copy is deep
    tbl["a"] = 99;
    tbl["sub"]["b"] = 77;
    EXPECT_EQ(tbl_copy["a"].get<int>(), 1);
    EXPECT_EQ(tbl_copy["sub"]["b"].get<int>(), 2);
}

TEST_F(LuaTest, ToStringNestedTable) {
    auto tbl = lua_.createTable();
    tbl["foo"] = 123;
    auto sub = lua_.createTable();
    sub["bar"] = 456;
    tbl["sub"] = sub;
    std::string s = lua::toString(tbl);
    EXPECT_NE(s.find("foo = 123"), std::string::npos);
    EXPECT_NE(s.find("bar = 456"), std::string::npos);
}

TEST_F(LuaTest, ParseVec2fUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec2f>(tbl["v2f_ut"]), vec2f(1.1F, 2.2F));
    EXPECT_EQ(lua::as<vec2f>(tbl["v2f_tbl"]), vec2f(1.1F, 2.2F));
    EXPECT_EQ(lua::as<vec2f>(tbl["v2f_num"]), vec2f(3.3F, 3.3F));
}

TEST_F(LuaTest, ParseVec3fUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec3f>(tbl["v3f_ut"]), vec3f(1.1F, 2.2F, 3.3F));
    EXPECT_EQ(lua::as<vec3f>(tbl["v3f_tbl"]), vec3f(1.1F, 2.2F, 3.3F));
    EXPECT_EQ(lua::as<vec3f>(tbl["v3f_num"]), vec3f(4.4F, 4.4F, 4.4F));
}

TEST_F(LuaTest, ParseVec4fUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec4f>(tbl["v4f_ut"]), vec4f(1.1F, 2.2F, 3.3F, 4.4F));
    EXPECT_EQ(lua::as<vec4f>(tbl["v4f_tbl"]), vec4f(1.1F, 2.2F, 3.3F, 4.4F));
    EXPECT_EQ(lua::as<vec4f>(tbl["v4f_num"]), vec4f(5.5F, 5.5F, 5.5F, 5.5F));
}

TEST_F(LuaTest, ParseVec2iUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec2i>(tbl["v2i_ut"]), vec2i(1, 2));
    EXPECT_EQ(lua::as<vec2i>(tbl["v2i_tbl"]), vec2i(3, 4));
    EXPECT_EQ(lua::as<vec2i>(tbl["v2i_num"]), vec2i(5, 5));
}

TEST_F(LuaTest, ParseVec3iUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec3i>(tbl["v3i_ut"]), vec3i(1, 2, 3));
}

TEST_F(LuaTest, ParseVec4iUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_EQ(lua::as<vec4i>(tbl["v4i_ut"]), vec4i(1, 2, 3, 4));
}

TEST_F(LuaTest, ParseRectfUsertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    EXPECT_TRUE(lua::as<Rectf>(tbl["rectf_ut"]).almostEqual(Rectf(1.1F, 2.2F, 3.3F, 4.4F), 1e-5F));
    EXPECT_TRUE(lua::as<Rectf>(tbl["rectf_tbl"]).almostEqual(Rectf(5.5F, 6.6F, 7.7F, 8.8F), 1e-5F));
}

TEST_F(LuaTest, ParseMat2Usertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    mat2f expected1{11.0F, 22.0F, 33.0F, 44.0F};
    mat2f from_lua = lua::as<mat2f>(tbl["mat2f_tbl"]);
    EXPECT_FLOAT_EQ(expected1[0][0], from_lua[0][0]);
    EXPECT_FLOAT_EQ(expected1[0][1], from_lua[0][1]);
    EXPECT_FLOAT_EQ(expected1[1][0], from_lua[1][0]);
    EXPECT_FLOAT_EQ(expected1[1][1], from_lua[1][1]);
}

TEST_F(LuaTest, ParseMat4Usertypes) {
    auto tbl = lua_.require("i/MathUT").as<sol::table>();
    mat4f expected1{1.0F, 3.0F, 5.0F, 7.0F, 9.0F, 11.0F, 13.0F, 15.0F, 17.0F, 19.0F, 21.0F, 23.0F, 25.0F, 27.0F, 29.0F, 31.0F};
    mat4f from_lua = lua::as<mat4f>(tbl["mat4f_tbl"]);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_FLOAT_EQ(expected1[i][j], from_lua[i][j]);
        }
    }
}
