premake5 gmake2
cd build/
make clean config=release_arm64
make -j4 config=release_arm64
cd ..
