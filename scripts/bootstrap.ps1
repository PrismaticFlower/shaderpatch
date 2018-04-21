# Setup developer environment.
.\scripts\setupdeveloperenvironment.ps1

# Clone in submodules.
git submodule update --init --recursive

# Build Compressonator
msbuild /t:Build /p:Configuration=Debug_MD /p:Platform=Win32 /m submodules\Compressonator\Compressonator\VS2015\CompressonatorLib.sln
msbuild /t:Build /p:Configuration=Release_MD /p:Platform=Win32 /m submodules\Compressonator\Compressonator\VS2015\CompressonatorLib.sln

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


del ".\buildtrees\" -Recurse -ErrorAction SilentlyContinue
del ".\packages\" -Recurse -ErrorAction SilentlyContinue

cd ../../
