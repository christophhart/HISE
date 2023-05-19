/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 10
  error: ""
  filename: "default_arg/default_2"
END_TEST_DATA
*/

float get5()
{
	return 5.0f;
}

int getDefault(int x=(int)get5())
{
	return x * 2;
}

int main(int input)
{
	return getDefault();
}

