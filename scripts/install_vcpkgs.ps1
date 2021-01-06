$Packages = "directxtex", "directxmesh", "openexr", "zlib", "stb", "glm", "ms-gsl", "nlohmann-json", "clara", "yaml-cpp", "abseil[cxx17]"
$x86Packages = $Packages + ("detours", "freetype", "fmt", "sol2", "lua[cpp]")
$x86ManagerPackages = "yaml-cpp"

./vcpkg install --triplet x64-windows $Packages
./vcpkg install --triplet x86-windows-static-md $x86Packages
./vcpkg install --triplet x86-windows-static $x86ManagerPackages
