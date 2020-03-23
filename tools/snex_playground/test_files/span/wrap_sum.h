/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 17
  error: ""
  filename: "span/wrap_sum"
END_TEST_DATA
*/


span<int, 8>::wrapped i = { 5 };

int main(int input)
{
	return input + i;
}

