/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "dsp/slice_4"
END_TEST_DATA
*/

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

using DynWrapType = dyn<float>::unsafe;

int main(int input)
{
	auto& d = slice(data, 0, 8);
	
	int numElements = 0;
	
	for(auto& s: d)
    {
        numElements++;
    }
	    
	return numElements;
}

