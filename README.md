# Allium

Allium is a strongly typed logic programming language.

It currently runs on Linux, MacOS, and Windows.

![Test Badge](https://github.com/jacobdweightman/allium/workflows/Run%20Tests/badge.svg)

## Getting Started

Some documentation is available in the [docs](docs/README.md) directory.
Coming soon: documentation and a tutorial on programming in Allium!

## Developing Allium

Allium can be built on Linux, MacOS, or Windows. It currently supports building with the Clang and MSVC++ compilers. Allium depends on several C++20 features, and thus requires relatively recent compiler versions. Building with GCC is not supported because of a bug in its coroutine implementation, but will be restored in a future release of GCC. When building on Linux, it is recommended to use Clang With libc++.

Build Allium with Cmake:
```
$ git clone https://github.com/jacobdweightman/allium.git
$ cd allium
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config $BUILD_TYPE
```

Run the interpreter:
```
$ ./allium/allium path/to/file.allium
```

Running the automated regression tests requires an LLVM development installation. In particular, LLVM's FileCheck must be in your path. All of the tests can be built and run like this:
```
$ cmake --build . --config $BUILD_TYPE --target unittests allium-ls-unittests
$ ctest --verbose
```

## Building the Compiler

We're bringing up an LLVM-based compiler for Allium. This will make is possible
to compile Allium programs into native executables and libraries for a variety
of hardware and operating systems! It's still very incomplete, so don't expect
to be able to build even simple programs with it yet.

Building the Allium with compiler is disabled by default, but can be enabled by
setting the `BUILD_COMPILER` variable in cmake. Here's a typical configuration
command:
```
$ mkdir build
$ cd build
$ cmake .. -DBUILD_COMPILER=1 -DCMAKE_BUILD_TYPE=Debug
```
