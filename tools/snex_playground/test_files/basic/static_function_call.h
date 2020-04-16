/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 25
  error: ""
  filename: "basic/static_function_call"
END_TEST_DATA
*/

static int getStaticInRoot()
{
	return 8;
}

struct X
{
	static const int z = 12;

	static int getStaticValue()
	{
		return z + 5;
	}
};


int main(int input)
{
	return X::getStaticValue() + getStaticInRoot();
}

