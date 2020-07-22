/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
  error: ""
  filename: "init/init_test10"
END_TEST_DATA
*/

struct X
{
	X(int input)
	{
		value = input * 2;
	}
	
	int value = 9;
};

span<span<X, 2>, 2> data = {{ 5 }};

int main(int input)
{
	return data[0][0].value + data[1][0].value;
}

