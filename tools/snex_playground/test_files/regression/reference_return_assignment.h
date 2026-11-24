/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 15
  output: 15
  error: ""
  filename: "regression/reference_return_assignment"
END_TEST_DATA
*/
int x = 90;
int& getRef()
{
	return x;
}

int test(int input)
{
	getRef() = input;
	return x;
}
