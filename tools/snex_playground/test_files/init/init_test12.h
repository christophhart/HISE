/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test12"
END_TEST_DATA
*/

int counter = 0;

struct X
{
	struct Y
	{
		Y()
		{
			value = 12;
		}
		
		int value = 0;
	};
	
	Y y;
};

X obj;

int main(int input)
{
	return obj.y.value;
}

