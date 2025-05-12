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

    // Try loading main.lua from assets.bin or assets/
    char* buffer = NULL;
    size_t size = 0;
    int loaded = 0;

    // First try assets.bin
    if (load_main_lua_from_bin("assets.bin", &buffer, &size)) {
        loaded = 1;
    } else {
        // Fallback to assets/
        if (load_main_lua_from_physfs("assets", &buffer, &size)) {
            loaded = 1;
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

    lua_close(L);
    PHYSFS_deinit();
    return 0;
}