#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#define LTYPE "int64"

#define LUATYPE64_STRING_SIZE 21  /* string to hold 18446744073709551615 */

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
/* Compatibility for Lua 5.1 */
static void
luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup+1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup + 1));
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3)); /* table must be below the upvalues, the name and the closure */
    }
    lua_pop(L, nup);  /* remove upvalues */
}
#endif

static int64_t
_toint64(lua_State* L, int index) {
    int64_t* v = (int64_t*)lua_touserdata(L, index);
    if (!v) {
        luaL_error(L, "bad argument #%d (%s expected, got %s)", index, "int64", lua_typename(L, lua_type(L, index)));
        return 0;
    }
    return *v;
}

static void *
_isint64(lua_State* L, int index) {
    void *p = lua_touserdata(L, index);
    if (p != NULL) {
        if (lua_getmetatable(L, index)) {
            luaL_getmetatable(L, LTYPE);
            if (!lua_rawequal(L, -1, -2))
                p = NULL;
            lua_pop(L, 2);
            return p;
        }
    }
    return NULL;
}

static int64_t
_checkint64(lua_State* L, int index) {
    int64_t* p;
    luaL_checktype(L, index, LUA_TUSERDATA);
    p = (int64_t*)luaL_checkudata(L, index, "int64");
    return p ? *p : 0;
}

static int64_t
_getint64(lua_State *L, int index) {
    char *end = NULL;
    switch(lua_type(L, index)) {
    case LUA_TNUMBER:
        return luaL_checknumber(L, index);
    case LUA_TSTRING:
        return strtoll(luaL_checkstring(L, index), &end, 10);
    case LUA_TLIGHTUSERDATA:
        return lua_touserdata(L, index);
    case LUA_TUSERDATA:
        if (_isint64(L, index))
            return (int64_t)_toint64(L, index);
    default:
        return _checkint64(L, index);
    }
}

static int64_t*
_pushint64(lua_State *L, int64_t n) {
    int64_t* p;
    luaL_checkstack(L, 2, "Unable to grow stack\n");
    p = (int64_t*)lua_newuserdata(L, sizeof(int64_t));
    *p = n;
    luaL_getmetatable(L, "int64");
    lua_setmetatable(L, -2);
    return p;
}

static int
int64_add(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    _pushint64(L, a + b);

    return 1;
}

static int
int64_sub(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    _pushint64(L, a - b);

    return 1;
}

static int
int64_mul(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    _pushint64(L, a * b);

    return 1;
}

static int
int64_div(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    if (b == 0) {
        return luaL_error(L, "div by zero");
    }
    _pushint64(L, a / b);

    return 1;
}

static int
int64_mod(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    if (b == 0) {
        return luaL_error(L, "mod by zero");
    }
    _pushint64(L, a % b);

    return 1;
}

static int64_t
_pow64(int64_t a, int64_t b) {
    int64_t a2;
    if (b == 1) {
        return a;
    }
    a2 = a * a;
    if (b % 2 == 1) {
        return _pow64(a2, b/2) * a;
    } else {
        return _pow64(a2, b/2);
    }
}

static int
int64_pow(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    int64_t p;
    if (b > 0) {
        p = _pow64(a,b);
    } else if (b == 0) {
        p = 1;
    } else {
        return luaL_error(L, "pow by nagtive number %d",(int)b);
    }
    _pushint64(L, p);

    return 1;
}

static int
int64_unm(lua_State *L) {
    int64_t a = _getint64(L,1);
    _pushint64(L, -a);
    return 1;
}

static int
int64_new(lua_State *L) {
    int64_t n;
    if (lua_gettop(L) >= 1) {
        switch (lua_type(L,1)) {
        case LUA_TNUMBER:
            n = _getint64(L,1);
            if (lua_gettop(L) == 2 && lua_type(L,2) == LUA_TNUMBER) {
                uint64_t l = n & __UINT64_C(0x00000000FFFFFFFF);
                uint64_t h = _getint64(L,2);
                h <<= 32; h &= __UINT64_C(0xFFFFFFFF00000000);
                n = h + l;
            }
            break;
        case LUA_TSTRING:
        case LUA_TUSERDATA:
            n = _getint64(L,1);
            break;
        default:
            luaL_argerror(L, 1, "int64_new: must be a number, (number, number) or string");
            break;

        }
    }
    _pushint64(L, n);
    return 1;
}

static int
int64_eq(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    printf("%s %s\n",lua_typename(L,1),lua_typename(L,2));
    printf("%ld %ld\n",a,b);
    lua_pushboolean(L,a == b);
    return 1;
}

static int
int64_lt(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    lua_pushboolean(L,a < b);
    return 1;
}

static int
int64_le(lua_State *L) {
    int64_t a = _getint64(L,1);
    int64_t b = _getint64(L,2);
    lua_pushboolean(L,a <= b);
    return 1;
}

static int
int64_len(lua_State *L) {
    int64_t a = _getint64(L,1);
    lua_pushnumber(L,(lua_Number)a);
    return 1;
}

static int
int64_higher(lua_State* L) {
    int64_t a = _getint64(L,1);
    int64_t b = a;
    lua_Number n = 0;
    n = (lua_Number)(int32_t)(((b & 0xFFFFFFFF00000000ULL) >> 32) & 0x00000000FFFFFFFFFULL);
    lua_pushnumber(L,n);
    return 1;
}

static int
int64_lower(lua_State* L) {
    int64_t a = _getint64(L,1);
    lua_pushnumber(L,(int32_t)(a & 0x00000000FFFFFFFFFULL));
    return 1;
}

static int
int64_tostring(lua_State *L) {
    int64_t a = _getint64(L,1);
    char s[LUATYPE64_STRING_SIZE];
    if (snprintf(s, LUATYPE64_STRING_SIZE, "%lld", a) < 0) {
        return luaL_error(L, "error writing int64 to a string");
    }
    lua_pushstring(L,s);
    return 1;
}

static const luaL_Reg int64_lib[] = {
    { "new", int64_new },
    { "higher", int64_higher },
    { "lower", int64_lower },
    { "tostring", int64_tostring },
    { NULL, NULL },
};

static const luaL_Reg int64_meta[] = {
    { "__add", int64_add },
    { "__sub", int64_sub },
    { "__mul", int64_mul },
    { "__div", int64_div },
    { "__mod", int64_mod },
    { "__unm", int64_unm },
    { "__pow", int64_pow },
    { "__eq", int64_eq },
    { "__lt", int64_lt },
    { "__le", int64_le },
    { "__len", int64_len },
    { "__tostring", int64_tostring },
    { NULL, NULL },
};

int
luaopen_int64(lua_State *L) {
    luaL_newmetatable(L, LTYPE);
    luaL_setfuncs(L, int64_meta, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_newtable(L);
    luaL_setfuncs(L, int64_lib, 0);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    return 1;
}
