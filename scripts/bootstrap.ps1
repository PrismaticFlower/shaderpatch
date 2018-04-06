git submodule update --init --recursive

# Bootstrap vcpkg and install packages.
cd submodules/vcpkg

./bootstrap-vcpkg.bat

./vcpkg install directxtex:x86-windows-static-md
./vcpkg install stb:x86-windows-static-md
./vcpkg install glm:x86-windows-static-md
./vcpkg install ms-gsl:x86-windows-static-md
./vcpkg install nlohmann-json:x86-windows-static-md
./vcpkg install boost:x86-windows-static-md
./vcpkg install clara:x86-windows-static-md
./vcpkg install yaml-cpp:x86-windows-static-md

