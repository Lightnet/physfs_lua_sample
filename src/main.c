#include <physfs.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_ASSETS 100
#define MAX_NAME 32

typedef struct {
    char magic[4]; // "ASST"
    uint32_t version;
    uint32_t num_assets;
    struct {
        char name[MAX_NAME];
        uint32_t offset;
        uint32_t size;
    } toc[MAX_ASSETS];
} AssetHeader;

// Lua binding for physfs.openRead
static int l_physfs_openRead(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    PHYSFS_File* file = PHYSFS_openRead(filename);
    if (!file) {
        lua_pushnil(L);
        lua_pushstring(L, PHYSFS_getLastError());
        return 2;
    }
    PHYSFS_File** ud = (PHYSFS_File**)lua_newuserdata(L, sizeof(PHYSFS_File*));
    *ud = file;
    luaL_getmetatable(L, "physfs_file");
    lua_setmetatable(L, -2);
    return 1;
}

// Lua binding for physfs.read
static int l_physfs_read(lua_State* L) {
    PHYSFS_File** ud = (PHYSFS_File**)luaL_checkudata(L, 1, "physfs_file");
    PHYSFS_File* file = *ud;
    size_t amount = luaL_optinteger(L, 2, PHYSFS_fileLength(file));

    char* buffer = malloc(amount);
    if (!buffer) {
        lua_pushnil(L);
        lua_pushstring(L, "Memory allocation failed");
        return 2;
    }

    PHYSFS_sint64 read = PHYSFS_readBytes(file, buffer, amount);
    if (read < 0) {
        free(buffer);
        lua_pushnil(L);
        lua_pushstring(L, PHYSFS_getLastError());
        return 2;
    }

    lua_pushlstring(L, buffer, read);
    free(buffer);
    return 1;
}

// Lua binding for physfs.close
static int l_physfs_close(lua_State* L) {
    PHYSFS_File** ud = (PHYSFS_File**)luaL_checkudata(L, 1, "physfs_file");
    PHYSFS_File* file = *ud;
    if (file) {
        PHYSFS_close(file);
        *ud = NULL; // Prevent double-close
    }
    return 0;
}

// Garbage collector for physfs_file
static int l_physfs_file_gc(lua_State* L) {
    PHYSFS_File** ud = (PHYSFS_File**)luaL_checkudata(L, 1, "physfs_file");
    if (*ud) {
        PHYSFS_close(*ud);
        *ud = NULL;
    }
    return 0;
}

// PhysFS module functions
static const struct luaL_Reg physfs_funcs[] = {
    {"openRead", l_physfs_openRead},
    {"read", l_physfs_read},
    {"close", l_physfs_close},
    {NULL, NULL}
};

// Metatable for physfs_file
static const struct luaL_Reg physfs_file_mt[] = {
    {"__gc", l_physfs_file_gc},
    {NULL, NULL}
};

// Register physfs module
static void luaopen_physfs(lua_State* L) {
    // Create physfs_file metatable
    luaL_newmetatable(L, "physfs_file");
    luaL_setfuncs(L, physfs_file_mt, 0);
    lua_pop(L, 1);

    // Create physfs module
    lua_newtable(L);
    luaL_setfuncs(L, physfs_funcs, 0);
    lua_setglobal(L, "physfs");
}

// Load assets from assets.bin into memory
int load_assets_from_bin(const char* bin_file, lua_State* L) {
    FILE* file = fopen(bin_file, "rb");
    if (!file) {
        printf("Failed to open %s: %s\n", bin_file, strerror(errno));
        return 0;
    }

    // Read header
    AssetHeader header;
    if (fread(&header, sizeof(AssetHeader), 1, file) != 1) {
        printf("Failed to read header from %s\n", bin_file);
        fclose(file);
        return 0;
    }
    if (strncmp(header.magic, "ASST", 4) != 0) {
        printf("Invalid asset file: %s\n", bin_file);
        fclose(file);
        return 0;
    }

    // Create assets table
    lua_newtable(L);
    for (uint32_t i = 0; i < header.num_assets; i++) {
        char* buffer = malloc(header.toc[i].size);
        if (!buffer) {
            printf("Memory allocation failed for %s\n", header.toc[i].name);
            fclose(file);
            return 0;
        }

        fseek(file, header.toc[i].offset, SEEK_SET);
        if (fread(buffer, 1, header.toc[i].size, file) != header.toc[i].size) {
            printf("Failed to read %s from %s\n", header.toc[i].name, bin_file);
            free(buffer);
            fclose(file);
            return 0;
        }

        lua_pushlstring(L, buffer, header.toc[i].size);
        lua_setfield(L, -2, header.toc[i].name);
        free(buffer);
    }

    fclose(file);
    lua_setglobal(L, "assets");
    return 1;
}

int load_main_lua_from_bin(const char* bin_file, char** buffer, size_t* size) {
    FILE* file = fopen(bin_file, "rb");
    if (!file) {
        printf("Failed to open %s: %s\n", bin_file, strerror(errno));
        return 0;
    }

    // Read header
    AssetHeader header;
    if (fread(&header, sizeof(AssetHeader), 1, file) != 1) {
        printf("Failed to read header from %s\n", bin_file);
        fclose(file);
        return 0;
    }
    if (strncmp(header.magic, "ASST", 4) != 0) {
        printf("Invalid asset file: %s\n", bin_file);
        fclose(file);
        return 0;
    }

    // Find main.lua
    for (uint32_t i = 0; i < header.num_assets; i++) {
        if (strcmp(header.toc[i].name, "main.lua") == 0) {
            *size = header.toc[i].size;
            *buffer = malloc(*size + 1);
            if (!*buffer) {
                printf("Memory allocation failed for main.lua\n");
                fclose(file);
                return 0;
            }

            fseek(file, header.toc[i].offset, SEEK_SET);
            if (fread(*buffer, 1, *size, file) != *size) {
                printf("Failed to read main.lua from %s\n", bin_file);
                free(*buffer);
                fclose(file);
                return 0;
            }
            (*buffer)[*size] = '\0'; // Null-terminate
            fclose(file);
            return 1;
        }
    }

    printf("main.lua not found in %s\n", bin_file);
    fclose(file);
    return 0;
}

int load_main_lua_from_physfs(const char* path, char** buffer, size_t* size) {
    if (!PHYSFS_mount(path, NULL, 1)) {
        printf("Failed to mount %s: %s\n", path, PHYSFS_getLastError());
        return 0;
    }

    if (!PHYSFS_exists("main.lua")) {
        printf("main.lua not found in %s\n", path);
        return 0;
    }

    PHYSFS_File* file = PHYSFS_openRead("main.lua");
    if (!file) {
        printf("Failed to open main.lua: %s\n", PHYSFS_getLastError());
        return 0;
    }

    *size = PHYSFS_fileLength(file);
    *buffer = malloc(*size + 1);
    if (!*buffer) {
        printf("Memory allocation failed for main.lua\n");
        PHYSFS_close(file);
        return 0;
    }

    if (PHYSFS_readBytes(file, *buffer, *size) != *size) {
        printf("Failed to read main.lua from %s\n", path);
        free(*buffer);
        PHYSFS_close(file);
        return 0;
    }
    (*buffer)[*size] = '\0'; // Null-terminate
    PHYSFS_close(file);
    return 1;
}

int file_exists(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Initialize PhysFS
    if (!PHYSFS_init(argv[0])) {
        printf("PhysFS init failed: %s\n", PHYSFS_getLastError());
        return 1;
    }

    // Initialize Lua
    lua_State* L = luaL_newstate();
    if (!L) {
        printf("Failed to initialize Lua\n");
        PHYSFS_deinit();
        return 1;
    }
    luaL_openlibs(L);

    // Register physfs module
    luaopen_physfs(L);

    // Try loading main.lua and assets
    char* buffer = NULL;
    size_t size = 0;
    int loaded = 0;
    int use_physfs = 0;

    // Check if assets.bin exists
    if (file_exists("assets.bin")) {
        if (load_assets_from_bin("assets.bin", L)) {
            if (load_main_lua_from_bin("assets.bin", &buffer, &size)) {
                loaded = 1;
            }
        }
    } else {
        // Fallback to assets/
        if (load_main_lua_from_physfs("assets", &buffer, &size)) {
            loaded = 1;
            use_physfs = 1;
        }
    }

    if (loaded) {
        // Execute Lua script
        if (luaL_loadstring(L, buffer) || lua_pcall(L, 0, 0, 0)) {
            printf("Lua error: %s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        free(buffer);
    } else {
        printf("Failed to load main.lua from any source\n");
    }

    if (!loaded || (!use_physfs && !file_exists("assets.bin"))) {
        printf("No valid asset source; Lua scripts may fail to access files\n");
    }

    lua_close(L);
    PHYSFS_deinit();
    return 0;
}