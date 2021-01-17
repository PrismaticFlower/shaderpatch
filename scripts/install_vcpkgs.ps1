$Packages = "directxtex", "directxmesh", "openexr", "zlib", "stb", "glm", "ms-gsl", "nlohmann-json", "clara", "yaml-cpp", "freetype", "abseil[cxx17]", "fmt"
$x86Packages = $Packages + ("detours", "lua[cpp]", "sol2")

./vcpkg install --triplet x64-windows-static $Packages
./vcpkg install --triplet x86-windows-static $x86Packages
