/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test2"
END_TEST_DATA
*/


struct X
{
	void init()
	{
		value = 12;
	}
	

	X()
	{
		init();
	}

	int value = 9;
};

X obj;

int main(int input)
{
	return obj.value;
}