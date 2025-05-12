# physfs_lua_sample

# license: MIT

# Information:
  This is for simple assets.bin for physical filesystem base on Quake 3's file subsystem. Read more on https://icculus.org/physfs

# Assets:
  Test files.
- data.bin is main.lua physfs virtual system.
- main.lua for default entry point.
- test.md testing file here.

# Debug Run

```
cd build
cmake --build . --target run --config Debug
```

# Save Assets:
cmd
```
cd build
./convert_assets.exe ./assets assets.bin
```
result
```
Created assets.bin with 1 assets
```
# Load Assets:
cmd
```
./load_assets.exe ./assets.bin
```

result
```
Assets in ./assets.bin (version 1, 1 assets):
- main.lua (offset: 4012, size: 201)
  First 4 bytes: 70 72 69 6e
```

# virtual filesystems

## Structure of assets.bin
 - Header:
    - magic: 4 bytes ("ASST")
    - version: 4 bytes (uint32_t, currently 1)
    - num_assets: 4 bytes (uint32_t, number of assets, e.g., 3 for data.bin, main.lua, test.md)
 - Table of Contents (TOC):
    - For each asset (up to MAX_ASSETS=100):
    - name: 32 bytes (MAX_NAME, null-terminated string, e.g., "data.bin")
    - offset: 4 bytes (uint32_t, offset from file start to asset data)
    - size: 4 bytes (uint32_t, asset data size in bytes)

For your assets.bin (from load_assets.exe output):
- Contains 3 assets: data.bin (4213 bytes), main.lua (433 bytes), test.md (11 bytes)
- Total TOC size: 3 * (32 + 4 + 4) = 120 bytes.
- Header size: 4 + 4 + 4 = 12 bytes.
- Data starts after header + TOC (offset 132).


## Why io.open Fails
- io.open uses standard C file I/O, which can’t access files inside assets.bin (a custom binary format) or PhysFS-mounted directories (virtual filesystems).
- PhysFS provides its own file access functions (PHYSFS_openRead, etc.), which we need to expose to Lua via a binding.

## Lua Binding Strategy
We’ll create a minimal Lua module (physfs) with functions:
 - physfs.openRead(filename): Opens a file for reading, returns a file handle.
 - physfs.read(file, amount): Reads up to amount bytes or all data, returns a string.
 - physfs.close(file): Closes the file handle.

These will wrap PHYSFS_openRead, PHYSFS_readBytes, and PHYSFS_close, respectively.

# Links:
 * https://mropert.github.io/2020/07/26/threading_with_physfs/
 * https://github.com/icculus/physfs
 * https://icculus.org/physfs/docs/html/index.html
