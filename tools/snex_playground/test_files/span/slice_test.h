/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "span/slice_test"
END_TEST_DATA
*/

span<int, 8> d = { 1, 2, 3, 4, 5, 6, 7, 8 };

int main(int input)
{
	auto refX = slice(d, 7, 2);
	
	// change the actual data
	refX[0] = 90;

	// should return 90...
	return d[2];
}

