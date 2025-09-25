#pragma once

#include <optional>
#include <functional>
#include <string>
#include <deque>

#include <lua.hpp>

typedef std::optional<std::string> LuaEnvError;
struct LuaEnvResult {
    LuaEnvError error;
    double result;
};

class LuaEnv {
public:
    LuaEnv();
    ~LuaEnv();

    LuaEnv(const LuaEnv&) = delete;
    LuaEnv& operator=(const LuaEnv&) = delete;

    LuaEnvError setPackagePath(const std::string& dir);

    LuaEnvError compile(const char* str);

    LuaEnvResult runInstance();

    inline bool hasInstance() const;

    std::optional<std::function<void(std::string)>> print_callback;

private:
    int compiledInstanceReference = LUA_NOREF;
    std::string originalPackagePath;
    lua_State* L;

    static int print_hook(lua_State* L);
};

static constexpr size_t LUAENV_OUTPUTLOG_MAX_MESSAGES = 20;

enum OutputLogMessageType {
    Text=0,
    Error=1
};

struct OutputLogMessage {
    std::string str;
    OutputLogMessageType type;
};

/// WARNING: Not thread-safe, use only for debugging
class OutputLog {
public:
    void add(const OutputLogMessage& message) {
        if (messages.size() == LUAENV_OUTPUTLOG_MAX_MESSAGES)
            messages.pop_front();

        messages.push_back(message);
    }

    std::deque<OutputLogMessage> messages;
};