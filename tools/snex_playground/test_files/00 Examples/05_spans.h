/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 21
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


int main(int input)
{
	int value = data[4];
	
	//return data[input];
}

