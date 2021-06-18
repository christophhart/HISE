/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 103
  error: ""
  filename: "init/init_test14"
END_TEST_DATA
*/

struct X
{
	X(int v)
	{
		value = v;
	}
	
	int value = 0;
};

int v = 91;

int main(int input)
{
	X obj = { v+12 };
	
	return obj.value;
}

