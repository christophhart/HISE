algorithms.hpp used to exist, and some functions are used some functions
in RTNeural. Later on, xsimd removed this file from the public API as
they were alternative implementation for C++17/20 standard APIs.
We are not replacing those functions to maintain compatibility with
older C++ versions. Hence importing this file here.

For more details, see: https://github.com/jatinchowdhury18/RTNeural/pull/81
