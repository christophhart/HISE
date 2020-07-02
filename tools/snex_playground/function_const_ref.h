/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "basic/function_const_ref"
END_TEST_DATA
*/

int other(const int& input)
{
    return input * 2;
}

int main(int input)
{
	int x = other(input);
	
	return x;
}

