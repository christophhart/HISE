/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test11"
END_TEST_DATA
*/

int counter = 0;

struct X
{
	X()
	{
		value = 2;
	
		counter++;
	}
	
	int value = 9;
};

span<span<X, 6>, 2> data;

int main(int input)
{
	return counter;
}

