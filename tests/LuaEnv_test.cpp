#include <gtest/gtest.h>

#include "../src/cpp/LuaEnv.h"

TEST(LuaEnvTest, CompileSuccess) {
    LuaEnv L;

    EXPECT_EQ(L.compile("return 42"), std::nullopt);
    EXPECT_EQ(L.compile("function a() print('Hello World') end return 1"), std::nullopt);
    EXPECT_EQ(L.compile("function a(b) return b * 2 end return a(4)"), std::nullopt);
}

TEST(LuaEnvTest, CompileError) {
    LuaEnv L;

    EXPECT_EQ(L.compile("print('Hello World!')"), std::nullopt);
    EXPECT_EQ(L.hasInstance(), true);
    EXPECT_NE(L.compile("ret 42"), std::nullopt);
    EXPECT_EQ(L.hasInstance(), false);
    EXPECT_NE(L.compile("function a() print('Hello World') end return 1 +"), std::nullopt);
    EXPECT_EQ(L.hasInstance(), false);
}

TEST(LuaEnvTest, RunWithoutCompile) {
    LuaEnv L;

    EXPECT_EQ(L.runInstance().error, std::make_optional("No compiled instance to run"));
}

TEST(LuaEnvTest, RunCompiledInstances) {
    LuaEnv L;

    EXPECT_EQ(L.compile("return 42"), std::nullopt);
    EXPECT_EQ(L.runInstance().error, std::nullopt);
    EXPECT_EQ(L.runInstance().result, 42.0); 
    EXPECT_EQ(L.compile("return 'hello world'"), std::nullopt);
    EXPECT_EQ(L.runInstance().error, std::nullopt);
    EXPECT_EQ(L.runInstance().result, 0.0); // return non-number returns 0.0
    EXPECT_EQ(L.compile("return 0.314"), std::nullopt);
    EXPECT_EQ(L.runInstance().error, std::nullopt);
    EXPECT_EQ(L.runInstance().result, 0.314); // return 0.314 returns 0.314
}

/// TODO: Add package path tests for linux and macOS
TEST(LuaEnvTest, SetPackagePathError) {
    LuaEnv L;

    EXPECT_NE(L.setPackagePath("nonexistent/path"), std::nullopt);
    EXPECT_NE(L.setPackagePath("C:\\Windows\\explorer.exe"), std::nullopt);
    EXPECT_NE(L.setPackagePath("C:\\lua.lua"), std::nullopt);
}

TEST(LuaEnvTest, SetPackagePathSuccess) {
    LuaEnv L;

    EXPECT_EQ(L.setPackagePath("C:\\Windows"), std::nullopt);
    EXPECT_EQ(L.setPackagePath("C:\\"), std::nullopt);
    EXPECT_EQ(L.setPackagePath("C:\\Users"), std::nullopt);
}

TEST(LuaEnvTest, PrintCallback) {
    LuaEnv L;

    std::string result = "deadbeef";

    L.compile("print('Hello World!')");
    L.runInstance();
    EXPECT_EQ(result, "deadbeef"); // Expect no runtime error if print_callback not set

    L.print_callback = [&result](std::string s) {
        result = s;
    };

    L.runInstance();
    EXPECT_EQ(result, "Hello World!");  // Expect print_callback is triggered without recompile

    L.compile("print(3+2, 'asdf')");
    L.runInstance();
    EXPECT_EQ(result, "5\tasdf"); // Expect typical case
}

TEST(LuaEnvTest, OutputLogTest) {
    LuaEnv L;
    OutputLog logger;

    L.print_callback = [&logger](std::string s) { logger.add({s, OutputLogMessageType::Text}); };

    L.compile("print(3+2, 'asdf')");
    L.runInstance();
    EXPECT_EQ(logger.messages.front().str, "5\tasdf");
    EXPECT_EQ(logger.messages.front().type, OutputLogMessageType::Text);
}

TEST(LuaEnvTest, OutputLogOverflow) {
    LuaEnv L;
    OutputLog logger;

    L.print_callback = [&logger](std::string s) { logger.add({s, OutputLogMessageType::Text}); };

    L.compile("print('foo')");
    L.runInstance();
    for (int i = 0; i < 500; i++) {
        L.compile("print('Hello World!')");
        L.runInstance();
    }
    L.compile("print('bar')");
    L.runInstance();
    EXPECT_EQ(logger.messages.size(), LUAENV_OUTPUTLOG_MAX_MESSAGES);
    EXPECT_EQ(logger.messages.front().str, "Hello World!");
    EXPECT_EQ(logger.messages.back().str, "bar");
}