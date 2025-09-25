add_executable(LuaEnv_test)
target_sources(LuaEnv_test
    PRIVATE
        LuaEnv_test.cpp
        ../src/cpp/LuaEnv.cpp
)
target_link_libraries(LuaEnv_test
    PRIVATE
        libluajit
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(LuaEnv_test)