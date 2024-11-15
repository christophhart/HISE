# HISE ONNX Dynamic library

This project will build a dynamic library that wraps the ONNX runtime and can be loaded dynamically (the official ONNX runtime release needs to be linked dynamically).

For now I won't include the compiled library so you have to build it yourself if you want to use that feature, but this might change in the future.

## Build instructions

Download the static library builds from [here](https://github.com/csukuangfj/onnxruntime-libs/releases) and move:

- the static library files into `Source/lib/Debug/` / `Source/lib/Release/` 
- the include directory to `Source/include` 

then open the projucer project and compile the dynamic libraries.

> Note that on macOS you might have to use an older release because of some weird C++ library errors. [That](https://github.com/csukuangfj/onnxruntime-libs/releases/download/v1.17.3/onnxruntime-osx-universal2-static_lib-1.17.3.zip) one worked for me (Universal binary).

If all went through fine, put the `.dll` / `.dylib` file into the this directory so that will be picked up by HISE.

## Deployment

If you're using the ONNX runtime in your compiled plugin, you must distribute this dynamic library with your plugin and copy it to your project's app data directory:

```
%APPDATA%/My Company/Product/onnx_hise_library.dll
```