/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 2
  output: 16
  error: ""
  filename: "basic/nested_function_call"
END_TEST_DATA
*/

int op1(int in)
{
	return in * 5;
}

int op2(int in2)
{
	return in2 + 1;
}

int f = 3;

int main(int input)
{
	return op2(op1(f));
}

