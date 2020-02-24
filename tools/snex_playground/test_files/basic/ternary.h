/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "basic/ternary"
END_TEST_DATA
*/

int main(int input)
{
	return input > 8 ? 5 : 9;
}

