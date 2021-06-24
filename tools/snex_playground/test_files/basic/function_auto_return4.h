/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "basics/function_auto_return4"
END_TEST_DATA
*/


struct X
{
	auto& get()
	{
		return v;
	}

	int v = 13;
};

int main(int input)
{
	X i;

	auto& v = i.get();
	
	v = 90;
	
	return i.get();
}

