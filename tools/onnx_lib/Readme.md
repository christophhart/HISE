# HISE ONNX Dynamic library

This project will build a dynamic library that wraps the ONNX runtime and can be loaded dynamically (the official ONNX runtime release needs to be linked dynamically).

## Build instructions

Download the static library builds from [here](https://github.com/csukuangfj/onnxruntime-libs/releases) and move the static library files into `Source/lib/Debug/` / `Source/lib/Release/` then open the projucer project and compile the dynamic libraries.

If all went through fine, put the `.dll` / `.dylib` file into the this directory so that will be picked up by HISE.

## Deployment

If you're using the ONNX runtime in your compiled plugin, you must distribute this dynamic library with your plugin and copy it to your project's app data directory:

```
%APPDATA%/My Company/Product/onnx_hise_library.dll
```