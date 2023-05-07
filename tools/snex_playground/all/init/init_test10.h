/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "init/init_test10"
END_TEST_DATA
*/

struct X
{
	X()
	{
		// this value will override the initialiser list
		value = 4;
	}
	
	// This will get overriden by the initialiser list
	int value = 9;
};

span<span<X, 2>, 2> data;

int main(int input)
{
	return data[0][0].value + data[1][0].value;
}

