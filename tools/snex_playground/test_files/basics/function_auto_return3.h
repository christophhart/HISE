/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "basics/function_auto_return3"
END_TEST_DATA
*/


struct X
{
	auto& get()
	{
		return data[0];
	}

	span<span<int, 2>, 2> data = { {0, 3} };

	int v = 13;
};

int main(int input)
{
	X i;

	auto& v = i.get();
	
	v[0] = 90;
	
	return i.data[0][0];
}

