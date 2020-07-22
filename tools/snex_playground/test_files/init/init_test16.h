/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "init/init_test16"
END_TEST_DATA
*/

struct X
{
	int value = 5;
};

struct Y
{
	Y(X& x)
	{
		x.value = 90;
	}
};


int main(int input)
{
	X obj;
	
	Y y = { obj };
	
	return obj.value;
}
