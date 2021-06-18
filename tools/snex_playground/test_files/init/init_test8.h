/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test8"
END_TEST_DATA
*/

struct X
{
	X()
	{
		value = 6;
	}
	
	int value = 9;
};

span<X, 2> data;

int main(int input)
{
	return data[0].value + data[1].value;
}

