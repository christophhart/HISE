/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "00 Examples/06_dyn"
END_TEST_DATA
*/

/** Introduction into the dyn type, a resizeable array
	that can be reassigned to different memory locations
*/

// First we define a span that creates a consecutive
// array of float numbers in memory.
span<float, 128> data = { 0.0f };


// A dyn object that we'll be pointing to the span 
// above
dyn<float> dynData;

int main(int input)
{
	// here we assign the dyn array to the
	// span data. It will be resized to 
	// match the data length of the span
	dynData.referTo(data, data.size());
	
	// Iterate over the dyn array
	// & means reference so you can change it.
	// (Remove the & and the test will fail)
	for(auto& s: dynData)
	{
		s = 12.0f;
	}
	
	// Since the dyn data operates on the
	// span data, the value of the first
	// span element has changed:
	return (int)data[0];
}

