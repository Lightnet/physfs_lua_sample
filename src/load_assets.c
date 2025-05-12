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
    if (argc != 2) {
        printf("Usage: %s <assets_bin>\n", argv[0]);
        return 1;
    }

    const char* bin_file = argv[1];

    // Initialize PhysFS
    if (!PHYSFS_init(argv[0])) {
        printf("PhysFS init failed: %s\n", PHYSFS_getLastError());
        return 1;
    }

    // Open assets.bin
    FILE* file = fopen(bin_file, "rb");
    if (!file) {
        printf("Failed to open %s: %s\n", bin_file, strerror(errno));
        PHYSFS_deinit();
        return 1;
    }

    // Read header
    AssetHeader header;
    fread(&header, sizeof(AssetHeader), 1, file);
    if (strncmp(header.magic, "ASST", 4) != 0) {
        printf("Invalid asset file: %s\n", bin_file);
        fclose(file);
        PHYSFS_deinit();
        return 1;
    }

    // List assets
    printf("Assets in %s (version %u, %u assets):\n", bin_file, header.version, header.num_assets);
    for (uint32_t i = 0; i < header.num_assets; i++) {
        printf("- %s (offset: %u, size: %u)\n", header.toc[i].name, header.toc[i].offset, header.toc[i].size);

        // Verify asset data (read first 4 bytes)
        fseek(file, header.toc[i].offset, SEEK_SET);
        char buffer[4];
        size_t read = fread(buffer, 1, sizeof(buffer), file);
        printf("  First 4 bytes: ");
        for (size_t j = 0; j < read; j++) {
            printf("%02x ", (unsigned char)buffer[j]);
        }
        printf("\n");
    }

    fclose(file);
    PHYSFS_deinit();
    return 0;
}