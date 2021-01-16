# Allium

Allium is a strongly typed logic programming language.

It currently runs on Linux, MacOS, and Windows.

![Test Badge](https://github.com/jacobdweightman/allium/workflows/Run%20Tests/badge.svg)

## Getting Started

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
$ ./allium path/to/file.log
```

Run the unit tests:
```
$ cmake --build . --target unittests
$ ./unittests
```
