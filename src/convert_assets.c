#include <physfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // Added for uint32_t

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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_dir> <output_bin>\n", argv[0]);
        return 1;
    }

    const char* input_dir = argv[1];
    const char* output_bin = argv[2];

    // Initialize PhysFS
    if (!PHYSFS_init(argv[0])) {
        printf("PhysFS init failed: %s\n", PHYSFS_getLastError());
        return 1;
    }

    // Mount input directory
    if (!PHYSFS_mount(input_dir, NULL, 1)) {
        printf("Failed to mount %s: %s\n", input_dir, PHYSFS_getLastError());
        PHYSFS_deinit();
        return 1;
    }

    // Collect asset files
    char** files = PHYSFS_enumerateFiles("/");
    if (!files) {
        printf("Failed to enumerate files: %s\n", PHYSFS_getLastError());
        PHYSFS_deinit();
        return 1;
    }

    AssetHeader header = { .magic = "ASST", .version = 1, .num_assets = 0 };
    for (char** file = files; *file; file++) {
        if (header.num_assets >= MAX_ASSETS) {
            printf("Too many assets (max %d)\n", MAX_ASSETS);
            break;
        }
        if (!PHYSFS_isDirectory(*file)) {
            strncpy(header.toc[header.num_assets].name, *file, MAX_NAME - 1);
            header.toc[header.num_assets].name[MAX_NAME - 1] = '\0';
            header.num_assets++;
        }
    }
    PHYSFS_freeList(files);

    // Open output file
    FILE* out = fopen(output_bin, "wb");
    if (!out) {
        printf("Failed to open %s: %s\n", output_bin, strerror(errno));
        PHYSFS_deinit();
        return 1;
    }

    // Write header placeholder
    fseek(out, 0, SEEK_SET);
    fwrite(&header, sizeof(AssetHeader), 1, out);

    // Write asset data and update TOC
    uint32_t offset = sizeof(AssetHeader);
    for (uint32_t i = 0; i < header.num_assets; i++) {
        PHYSFS_File* in = PHYSFS_openRead(header.toc[i].name);
        if (!in) {
            printf("Failed to open %s: %s\n", header.toc[i].name, PHYSFS_getLastError());
            fclose(out);
            PHYSFS_deinit();
            return 1;
        }

        PHYSFS_sint64 size = PHYSFS_fileLength(in);
        header.toc[i].size = (uint32_t)size;
        header.toc[i].offset = offset;

        char* buffer = malloc(size);
        PHYSFS_readBytes(in, buffer, size);
        fwrite(buffer, 1, size, out);
        free(buffer);
        PHYSFS_close(in);

        offset += size;
    }

    // Rewrite header with updated TOC
    fseek(out, 0, SEEK_SET);
    fwrite(&header, sizeof(AssetHeader), 1, out);

    fclose(out);
    PHYSFS_deinit();
    printf("Created %s with %u assets\n", output_bin, header.num_assets);
    return 0;
}