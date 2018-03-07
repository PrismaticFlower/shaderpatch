git submodule update --init --recursive

# Bootstrap vcpkg and install packages.
cd submodules/vcpkg

./bootstrap-vcpkg.bat

./vcpkg install directxtex:x86-windows
./vcpkg install glm:x86-windows-static
./vcpkg install ms-gsl:x86-windows-static
./vcpkg install nlohmann-json:x86-windows-static

