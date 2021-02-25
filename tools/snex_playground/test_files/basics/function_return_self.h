/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 13
  error: ""
  filename: "basics/function_return_self"
END_TEST_DATA
*/


struct X
{
	auto& getSelf()
	{
		return *this;
	}

	int value = 13;
};

int main(int input)
{
	X i;

	return i.getSelf().value;
}

