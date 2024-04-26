if [ -d "build" ]; then
    rm -r build
fi

mkdir build && cd build
cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
cmake --preset=default ..
