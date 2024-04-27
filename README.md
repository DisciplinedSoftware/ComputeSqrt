# ComputeSqrtOf42

This is a simple executable that display and compute the square root of 42 in different ways.

This project requires C++20 or more.

Dependencies are managed by vcpkg.

Tests are done using the [catch2](https://github.com/catchorg/Catch2) framework.

## How to execute

> For simplicity, you should run it using GitHub [codespaces](https://github.com/features/codespaces) as everything as been prepared in a Docker container.

Run the following commands in the terminal.

``` bash
{
./generate.sh
cmake --build build
./build/ComputeSqrtOf42
}
```

> The command ./generate.sh is used to generate the build files and it only needs to be run once.

