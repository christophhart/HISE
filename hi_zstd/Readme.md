# zstd JUCE Module

This module wraps the new compression algorithm `zstd` from Facebook in convenient classes for usage within a JUCE project.
There are `InputStream` / `OutputStream` compatible interfaces as well as dedicated compressor classes for different use cases.
The library uses the `zstd` namespace and tucks all library code in the .cpp files to keep the compile time as short as possible.




## Automatic Type conversion

A handy feature of the `ZCompressor` class is the native conversion between selected JUCE classes which makes writing the code as smooth as possible. Currently supported are:

- `ValueTree`
- `String`
- `File`
- `MemoryBlock`

### Example

```cpp
ZCompressor comp;

// Convert a compressed File to a ValueTree:
ValueTree uncompressed;
File compressedFile = File("SomeCompressedFile.dat");
comp.expandFromType(compressedFile, uncompressed);

// Convert a uncompressed File to a compressed MemoryBlock
File uncompressedFile = File("SomeUncompressedFile.dat");
MemoryBlock compressedData;
comp.compressFromType(uncompressedFile, compressedData);

```


## Dictionaries

The most interesting feature of zstd is it's use of dictionaries, which are precalculated compression tables that accelerate and increase the compression ratio for small files. If you know the data you are about to compress, you can generate a dictionary, store it as binary data and use it for the compression to benefit from this feature. All you have to do is to:

1. Create the dictionary using either `zstd` or the functions in this module.
2. Write a `DictionaryProvider` class that returns a MemoryBlock with the dictionary. 
3. Pass this as template argument to the `ZCompressor` class.

### Example

```cpp

class MySpecialDataDictionaryProvider: public ProviderBase<void>
{
public:
    MySpecialDataDictionaryProvider():
       ProviderBase(nullptr)
    {};
    
    MemoryBlock createDictionary() override
    {
        // Use the BinaryData builder from the Projucer for this
        static const char dictionary[] = [100, 24, 254, ...]; 
        
        return MemoryBlock(dictionary, sizeof(dictionary));
    }
}

// Later...

ZCompressor<MySpecialDataDictionaryProvider> compressor;
compressor.expandFromType(x, y);
```

## License

The module is licensed under the same license as zstd (BSD / GPL), so feel free to use it how ever you like.

