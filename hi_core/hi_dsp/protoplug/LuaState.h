#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#define REQUIRE_JIT 1

// lua types
#define LUA_TNIL			0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER			3
#define LUA_TSTRING			4
#define LUA_TTABLE			5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD			8
#define LUA_GLOBALSINDEX	(-10002)


namespace protolua
{
// from the lua headers :
typedef struct { } lua_State;
typedef double lua_Number;
typedef int (*lua_CFunction) (lua_State *L);

// some function pointer types :
typedef lua_State*	(*ptr_luaL_newstate)		();
typedef void		(*ptr_luaL_openlibs)		(lua_State *L);
typedef int			(*ptr_luaL_loadbuffer)		(lua_State *L, const char *buff, size_t sz, const char *name);
typedef int			(*ptr_luaL_loadstring)		(lua_State *L, const char *s);
typedef const char* (*ptr_lua_tolstring)		(lua_State *L, int idx, size_t *len);
typedef lua_Number	(*ptr_lua_tonumber)			(lua_State *L, int idx);
typedef int			(*ptr_lua_toboolean)		(lua_State *L, int idx);
typedef void		(*ptr_lua_pushcclosure)		(lua_State *L, lua_CFunction fn, int n);
typedef void		(*ptr_lua_close)			(lua_State *L);
typedef int			(*ptr_lua_gettop)			(lua_State *L);
typedef void		(*ptr_lua_settop)			(lua_State *L, int idx);
typedef int			(*ptr_lua_pcall)			(lua_State *L, int nargs, int nresults, int errfunc);
typedef void		(*ptr_lua_getfield)			(lua_State *L, int idx, const char *k);
typedef void		(*ptr_lua_pushvalue)		(lua_State *L, int idx);
typedef void		(*ptr_lua_pushlightuserdata)(lua_State *L, void *p);
typedef void		(*ptr_lua_pushstring)		(lua_State *L, const char *s);
typedef void		(*ptr_lua_pushnumber)		(lua_State *L, lua_Number n);
typedef void		(*ptr_lua_pushboolean)		(lua_State *L, int b);
typedef int			(*ptr_lua_type)				(lua_State *L, int idx);
typedef void		(*ptr_lua_setfield)			(lua_State *L, int idx, const char *k);
typedef int			(*ptr_lua_isstring)			(lua_State *L, int idx);
typedef int			(*ptr_lua_isnumber)			(lua_State *L, int idx);
typedef const char* (*ptr_lua_typename)			(lua_State *L, int t);
typedef void *		(*ptr_lua_newuserdata)		(lua_State *L, size_t sz);
#if REQUIRE_JIT
typedef int			(*ptr_luajit_setmode)		(lua_State *L, int idx, int mode);
#endif


// the purpose of this abominable class is to load a lua/luajit dynamic library
// located at a chosen path at runtime and resolve its symbols, while remaining cross platform
// and giving a clear message if the library is missing.
// i don't really care about encapsulating lua in a c++ class but it happened as collateral damage
class LuaState
{
public:
	LuaState(File defaultDir);
	~LuaState();
	void openlibs();
	int loadbuffer(const char *buff, size_t sz, const char *name);
	int loadstring(const char *s);
	const char * tolstring(int idx, size_t *len);
	lua_Number tonumber(int idx);
	int toboolean(int idx);
	void pushcclosure(lua_CFunction fn, int n);
	void close();
	int gettop();
	void settop(int idx);
	int pcall(int nargs, int nresults, int errfunc);
	void getfield(int idx, const char *k);
	void pushvalue(int idx);
	void pushlightuserdata(void *p);
	void pushstring(const char *s);
	void pushnumber(lua_Number n);
	void pushboolean(int b);
	int type(int idx);
	void setfield(int idx, const char *k);
	int isstring (int idx);
	int  isnumber(int idx);
	const char * ltypename(int t);
	void * newuserdata(size_t sz);
#if REQUIRE_JIT
	int jit_setmode(int idx, int mode);
#endif

	void setglobal(const char * n) {
		setfield(LUA_GLOBALSINDEX, n);
	}
	void getglobal(const char * n) {
		getfield(LUA_GLOBALSINDEX, n);
	}
	void pop(int n) {
		settop(-(n)-1);
	}
	const char *  tostring(int idx) {
		return tolstring(idx, 0);
	}
	bool isfunction(int idx) {
		return lua_type(l, idx) == LUA_TFUNCTION;
	}
	bool isboolean(int idx) {
		return lua_type(l, idx) == LUA_TBOOLEAN;
	}

	lua_State *l;
	bool failed;
	String errmsg;

	SharedResourcePointer<DynamicLibrary> dll;
	static ptr_luaL_newstate	luaL_newstate;
	static ptr_luaL_openlibs	luaL_openlibs;
	static ptr_luaL_loadbuffer	luaL_loadbuffer;
	static ptr_luaL_loadstring	luaL_loadstring;
	static ptr_lua_tolstring	lua_tolstring;
	static ptr_lua_tonumber		lua_tonumber;
	static ptr_lua_toboolean	lua_toboolean;
	static ptr_lua_pushcclosure	lua_pushcclosure;
	static ptr_lua_close		lua_close;
	static ptr_lua_gettop		lua_gettop;
	static ptr_lua_settop		lua_settop;
	static ptr_lua_pcall		lua_pcall;
	static ptr_lua_getfield		lua_getfield;
	static ptr_lua_pushvalue	lua_pushvalue;
	static ptr_lua_pushlightuserdata	lua_pushlightuserdata;
	static ptr_lua_pushstring	lua_pushstring;
	static ptr_lua_pushnumber	lua_pushnumber;
	static ptr_lua_pushboolean	lua_pushboolean;
	static ptr_lua_type			lua_type;
	static ptr_lua_setfield		lua_setfield;
	static ptr_lua_isstring		lua_isstring;
	static ptr_lua_isnumber		lua_isnumber;
	static ptr_lua_typename		lua_typename;
	static ptr_lua_newuserdata	lua_newuserdata;
#if REQUIRE_JIT
	static ptr_luajit_setmode	luajit_setmode;
#endif
};

}
