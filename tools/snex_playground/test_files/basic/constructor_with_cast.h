/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: double
  input: 12
  output: 24
  error: ""
  filename: "basics/constructor_with_cast"
END_TEST_DATA
*/

struct X
{
	X(int a)
	{
		value = a * 2;
	}
	
	int value = 5;
};


int main(double input)
{
	auto v = (int)input;

	X obj((int)v);
	
	return obj.value;
}

