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
	X(int input)
	{
		value = input;
	
		counter++;
	}
	
	int value = 9;
};

span<span<X, 6>, 2> data = {{ 5 }};

int main(int input)
{
	return counter;
}

