$Packages = "directxtex", "directxmesh", "openexr", "zlib", "stb", "glm", "ms-gsl", "nlohmann-json", "clara", "yaml-cpp"
$x86Packages = $Packages + ("detours")

./vcpkg install $Packages
./vcpkg install --triplet x86-windows $x86Packages
