/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 1
  output: 4
  error: ""
  filename: "basic/simple_ternary"
END_TEST_DATA
*/

int main(int input)
{
	return input == 0 ? 5 : 4;
}

