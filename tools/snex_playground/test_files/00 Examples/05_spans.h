/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 14
  output: 2
  error: ""
  filename: "00 Examples/05_spans"
END_TEST_DATA
*/

/** Introduction into the span, a fixed-sized array

	that can be used to create multidimensional data structures
*/


// An array of 12 integers that are initialised with 19
span<int, 12> data = { 19 };



// An array of 4 float values with different initial values
span<float, 4> fData = { 0.0f, 1.0f, 2.0f, 3.0f };


// We can define Aliases for complex type definitions
// and use them later
using StereoFrame = span<float, 2>;

// This is a (multidimensional) array using the
// using alias defined above.
span<StereoFrame, 128> stereoChannels = { {0.0f, 2.0f} };


int main(int input)
{
	// If you put in a literal as index,
	// it will check at compile time if the
	// index is out of bounds.
	// Change the number below to something >12
	// and you will see an error message
	// indicating out of bounds
	int value = data[11];
	
	
	// If you want to use a dynamic index
	// you will need to create an index type
	// for the target array
	// with a specific out-of-bounds behaviour
	index::wrapped<0, false> index;
	
	// Now we assign the dynamic input to the
	// index type. Then we can use the index
	// variable as subscription index without
	// a crash
	index = input;
	
	// The index will now wrap around the
	// array size. If the value is above the 
	// highest index, it will wrap around. This
	// is a common use case for delay buffers etc.
	return (int)fData[index];
}

