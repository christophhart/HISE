
# SNEX Compiler Test suite

This directory contains test files that are supposed to ensure the correct compiler functionality.

## Directory structure

There are multiple files in different subdirectories that test different aspects of the compiler. There is one special directory called `audio_files` which contains small chunks of audio data that is used by some test routines.

## File structure

The test files are fully standard conform C++ files (the SNEX built in complex types can be imported with the SNEX extension header). They all contain one entry function and some metadata about the input and expected result.

### File metadata

At the top of each file there is a comment block which contains the metadata used by the test. This is an example:

```cpp
/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int int
  input: 12 4
  output: 16
  error: "This is a expected error message"
  filename: "basic/pre_inc"
  [events: { JSON with event data }]
END_TEST_DATA
*/
```

Each line is a metadata key/pair value, with the syntax `key: value`. Multiple values are delimited by a single whitespace, quoted strings are preserved as one value.


| Key | Value | Description |
| --- | --- | --- |
| `f` | function ID | The name of the entry function that is supposed to be called. |
| `ret` | type  | the return type of the function (can be `int`, `float`, `double` or `block` |
| `args` | list of types | a whitespace separated list of types (like the return type) |
| `output` | value | either a literal value for simple types or a filename for `block`
| `input` | list of values | a whitespace separated list of values (like the output) |
| `error` | quoted String | the exact error message that is supposed to be thrown. This can be used to test the compiler against invalid input and expect that it behaves correctly and prints the expected error message. If this is not an empty String (`""`), the test result will not compare the output, but just check that the two error messages are identical |
| `events` | JSON | (optional) list of events for the process data testmode
| `compile_flags` | String list | (optional) a whitespace separated list of optimizations (as they are shown in the SNEX playground that you can use to skip the test if this optimization isn't enabled. |
| `loop_count` | int | (optional) the expected number of loops. | 
| `filename` | a relative filename | if this is not empty, this filename will be overriden with the current code. This is used by the SNEX Playground's **Test Mode** to quickly generate new tests. |


### Input / Output values

the `input` and `output` values can be either literal values for simple types (`int`, `float`, or `double`) or a relative audio file name with quote delimiters. If this is the case, then the test will look in the `audio_files` directory for a respective file and pass the content of it as block into the function.

If the `output` value is a filename and the return type is a block, the file will be compared against the block that was fed into the function after the execution of the function. Since floating point numbers tend to be never 100% bit-equal, there is an error margin of about -80dB to avoid false positives.

> If the file does not exist, it will not compare it, but just create the file and skip the test. This can be used to create new test signals that are supposed to be checked at 
the next test run.

### ProcessData test mode

If you want to use the `ProcessData` type for testing "real-world input" including multichannel audio buffers and events, you can do so by specifying `ProcessData<NumChannels>` as args type. 

> This test mode opposes some restrictions on the function signature:  
> It must be `int main(ProcessData<NumChannels>& data)`.

In this mode the input must be a String that points to an existing audio file with the same channel amount as `NumChannels`. Use the `zero1.wav` to `zero16.wav` to quickly create empty process datas. The output must be the output file name that the processed signal will be compared against (if it doesn't exist, it will create this file for later comparison).

The return `int` value is always supposed to be zero, so you can add additional tests in the code and return a non-zero value if they fail.

### Node test mode

If you want to test a node just like it will be used in scriptnode, you can specify the 
typename of the node within brackets as `f` property: f: {MyNodeThatShouldBeTested}.

In this mode, the code will be surrounded by the scriptnode boilerplate code (everything will be tucked into a `impl` namespace, then an object with the given type is being created and wrapper functions for all node callbacks will be added on the root level.

It will call the functions in their correct order using the processing specifications from the input file:

- prepare();
- reset();
- process()

> In this mode the input and output will be automatically set to audio files.

#### Adding events

If you want to add events to the ProcessData, you can do so by supplying a JSON object with the event data as `events` key. It expects this format:

```
[
	EventObject1,
	EventObject2
]
```

And each `EventObject` needs to has these properties:

| Key | Expected Value | Description |
| --- | ---------- | --------- |
| `Type` | NoteOn, NoteOff, Controller | String indicating the event type |
| `Channel` | int | the event channel from 1 - 255 |
| `Value1` | int | the first value (for notes it's the note number) |
| `Value2` | int | the second value (for notes it's the velocity) |
| `Timestamp` | int | the timestamp in the buffer. Must be less than the buffer size and it must be aligned to the `HISE_EVENT_RASTER` value |

## Add required compile flags

Some tests that check specific compiler features (eg. auto vectorisation) can be skipped if the optimization is not enabled.

> Be aware that this function is reserved for special cases (eg. manual calls to `toSimd()` which wouldn't compile otherwise) and must not be used to cheat your way into a passing test suite...

## Loop Counter

In order to test the loop optimisations you can enter a number of loops that this test is supposed to create. This can be used to check whether the loops got optimised away or being unrolled. It just checks the ASM output for the number of occurrences of the `loop {` String. If the number doesn't match, the test will fail.

> Make sure to use the correct number (if you inline functions, it will duplicate the loop code). Also this check will only work if you have supplied `LoopOptimisation` as required compile flag...

## Use template `T` for testing all numeric types

If you specify `T` as input and return type, the test will be executed for all numeric types (`int`, `double` and `float`). It adds the line 

```cpp
using T = int;
```

at the beginning of the code, so make sure you cast every literal value to `T` to avoid warnings.

## How to create new tests

The SNEX playground can be used to quickly create new tests. If it's compiled with the `SNEX_PLAYGROUND_TEST_MODE`, it will create a test template and execute the current code snippet and just prints the test result. As soon as the `filename` key is specified, it will write the compiled code to the specified file.
