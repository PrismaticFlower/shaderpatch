$Packages = "directxtex", "directxmesh", "openexr", "zlib", "stb", "glm", "ms-gsl", "nlohmann-json", "clara", "yaml-cpp", "freetype"
$x86Packages = $Packages + ("detours", "abseil[cxx17]", "fmt")
$x86ManagerPackages = "yaml-cpp"

./vcpkg install --triplet x64-windows $Packages
./vcpkg install --triplet x86-windows-static-md $x86Packages
./vcpkg install --triplet x86-windows-static $x86ManagerPackages
