/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "init/init_test13"
END_TEST_DATA
*/

int counter = 0;

struct X
{
	X()
	{
		value = 1;
	}
	
	int value = 0;
};


int main(int input)
{
	X obj, obj2;
	
	return obj.value + obj2.value;
}

