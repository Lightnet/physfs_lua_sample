print("Hello from main.lua!")

if assets and assets["data.bin"] then
    -- Using assets.bin (in-memory)
    print("Loaded data.bin, size: " .. #assets["data.bin"] .. " bytes")
else
    -- Fallback to physfs (assets/)
    local file, err = physfs.openRead("data.bin")
    if not file then
        print("Failed to open data.bin: " .. (err or "unknown error"))
    else
        local data, err = physfs.read(file)
        if not data then
            print("Failed to read data.bin: " .. (err or "unknown error"))
        else
            print("Loaded data.bin, size: " .. #data .. " bytes")
        end
        physfs.close(file)
    end
end