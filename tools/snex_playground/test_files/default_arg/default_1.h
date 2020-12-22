/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "default_arg/default_1"
END_TEST_DATA
*/

int getDefault(int x=4)
{
	return x * 2;
}

int main(int input)
{
	return getDefault();
}

