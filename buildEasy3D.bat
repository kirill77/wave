pushd ..

git clone https://github.com/LiangliangNan/Easy3D.git --recurse-submodules
cd Easy3D

git checkout a8a82e3132ef2aab747b328103822d74f7ae1bc5
git submodule update --init --recursive

mkdir build
cd build
cmake ..

popd