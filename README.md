# Allium

Allium is a strongly typed logic programming language.

It currently runs on Linux, MacOS, and Windows.

![Test Badge](https://github.com/jacobdweightman/allium/workflows/Run%20Tests/badge.svg)

## Getting Started

Coming soon: documentation and a tutorial on programming in Allium!

## Developing Allium

Allium can be built on Linux, MacOS, or Windows. It currently supports building with the Clang and MSVC++ compilers. Allium depends on several C++20 features, and thus requires relatively recent compiler versions. Building with GCC is not supported because of a bug in its coroutine implementation, but will be restored in a future release of GCC. When building on Linux, it is recommended to use Clang With libc++.

Build Allium with Cmake:
```
$ git clone https://github.com/jacobdweightman/allium.git
$ cd allium
$ mkdir build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config $BUILD_TYPE
```

Run the interpreter:
```
$ ./allium path/to/file.allium
```

Run the unit tests:
```
$ cmake --build . --target unittests
$ ./unittests
```
