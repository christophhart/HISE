/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 180
  error: ""
  filename: "init/init_test5"
END_TEST_DATA
*/

struct Y
{
	int yValue = 90;
};


struct X
{
	X(int o)
	{
		value = o * 2;
	}

	int value = 9;
};

Y yObj;
X obj = { yObj.yValue };

int main(int input)
{
	return obj.value;
}