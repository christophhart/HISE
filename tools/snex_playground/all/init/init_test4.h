/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 180
  error: ""
  filename: "init/init_test4"
END_TEST_DATA
*/

struct Y
{
	int yValue = 90;
};


struct X
{
	X(Y& o)
	{
		value = o.yValue * 2;
	}

	int value = 9;
};

Y yObj;
X obj = { yObj };

int main(int input)
{
	return obj.value;
}