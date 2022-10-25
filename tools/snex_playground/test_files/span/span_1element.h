/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "span/span_1element"
END_TEST_DATA
*/

int main(int input)
{
	span<int, 1> obj = { 3 };
	
	return obj[0];
}

