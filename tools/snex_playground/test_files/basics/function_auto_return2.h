/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 13
  error: "Line 28(18): Can't assign non-reference"
  filename: "basics/function_auto_return2"
END_TEST_DATA
*/


struct X
{
	auto get()
	{
		return v;
	}

	int v = 13;
};

X i;

int main(int input)
{
	auto& z = i.get();
	z = 15;
	return i.v;
}

