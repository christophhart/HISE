/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "basic/function_with_same_parameter_name"
END_TEST_DATA
*/

int other(int input)
{
    return input * 2;
}

int main(int input)
{
	return other(input + 3);
}

