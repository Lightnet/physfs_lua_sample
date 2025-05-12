print("Hello from main.lua!")
local file = io.open("data.bin", "rb")
if file then
    local data = file:read("*all")
    print("Loaded data.bin, size: " .. #data .. " bytes")
    file:close()
end