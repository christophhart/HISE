/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "init/init_test3"
END_TEST_DATA
*/


struct X
{
	X(int v)
	{
		value = v * 2;
	}

	int value = 9;
};

X obj = { 4 };

int main(int input)
{
	return obj.value;
}