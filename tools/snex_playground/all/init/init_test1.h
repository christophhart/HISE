/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test1"
END_TEST_DATA
*/


struct X
{
	X()
	{
		value = 12;
	}

	int value = 9;
};

X obj;

int main(int input)
{
	return obj.value;
}