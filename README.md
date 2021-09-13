# CodeGen

A fork of @pdziepak's experimental wrapper over LLVM for generating and compiling code at run-time.

## New Features
* Supports LLVM 12
* Requires C++ 20 (modeled with C++ concepts)
* Header-only library (With dependencies on various LLVM libraries)
* Supports aggregate types: arrays, structs (under construction)

## Future Plans
* Remodel `values`, `constants`, and various of operations with C++ class hierarchy.
* Better support of native types.
* Better versatility: static LLVM IR codegen; better debuggability; more supporting functions.
