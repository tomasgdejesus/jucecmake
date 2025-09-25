#include "LuaEnv.h"

#include <filesystem>
#include <algorithm>
#include <sstream>

LuaEnv::LuaEnv() {
    L = luaL_newstate();
    luaL_openlibs(L);

    /// TODO: Setup Lua env
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    originalPackagePath = lua_tostring(L, -1);
    lua_pop(L, 1);

    // hook print() value
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, print_hook, 1);
    lua_setglobal(L, "print");
}

LuaEnv::~LuaEnv() {
    lua_close(L);
}

LuaEnvError LuaEnv::setPackagePath(const std::string& dir) {
    std::filesystem::path p(dir);
    if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p))
        return std::make_optional("Path not valid: " + dir);

    lua_getglobal(L, "package");
    lua_pushstring(L, (originalPackagePath + dir + "?.lua").c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    return std::nullopt;
}

LuaEnvError LuaEnv::compile(const char* str) {
    // Remove old compiled instance if it exists
    if (compiledInstanceReference != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, compiledInstanceReference);

    if (luaL_loadstring(L, str) != LUA_OK) {
        // Failed
        auto ret = std::make_optional(lua_tostring(L, -1));
        lua_pop(L, 1); // pop err msg
        compiledInstanceReference = LUA_NOREF;
        return ret;
    }

    compiledInstanceReference = luaL_ref(L, LUA_REGISTRYINDEX);
    
    return std::nullopt;
}

LuaEnvResult LuaEnv::runInstance() {
    if (!hasInstance())
        return { std::make_optional("No compiled instance to run"), 0.0 };

    lua_rawgeti(L, LUA_REGISTRYINDEX, compiledInstanceReference);

    LuaEnvResult result;
    result.error = lua_pcall(L, 0, 1, 0) != LUA_OK ? std::make_optional(lua_tostring(L, -1)) : std::nullopt;

    if (!result.error.has_value()) {
        if (lua_isnumber(L, -1))
            result.result = lua_tonumber(L, -1);
        else
            result.result = 0.0;
    }

    lua_pop(L, 1); // Pop the result or error message
    return result;
}

inline bool LuaEnv::hasInstance() const {
    return compiledInstanceReference != LUA_NOREF;
}

int LuaEnv::print_hook(lua_State* L) {
    LuaEnv* env = static_cast<LuaEnv*>(lua_touserdata(L, lua_upvalueindex(1)));

    if (!env->print_callback)
        return 0;

    int nargs = lua_gettop(L);

    std::stringstream out_stream;

    /* https://stevedonovan.github.io/lua-5.1.4/lbaselib.c.html */
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= nargs; i++) {
        const char* s;
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        s = lua_tostring(L, -1);
        if (!s)
            return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
        if (i > 1)
            out_stream << '\t';
        out_stream << s;
        lua_pop(L, 1);
    }
    (*env->print_callback)(out_stream.str());
    return 0;
}