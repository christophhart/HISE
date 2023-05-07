/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "init/init_test20"
END_TEST_DATA
*/

struct X
{
	X()
	{
		constructedValue = 5;
	}
	
	int constructedValue = 1;
	int initValue = 4;
};


X obj;

int main(int input)
{
	X obj2;
	
	return obj.constructedValue + obj.initValue +
	       obj2.constructedValue + obj2.initValue;
}


