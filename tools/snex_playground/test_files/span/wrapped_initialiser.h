/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "span/wrapped_initialiser"
END_TEST_DATA
*/

span<int, 5>::wrapped s = { 7 };

int main(int input)
{
	return s;
}

