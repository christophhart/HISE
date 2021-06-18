/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1296
  error: ""
  filename: "init/init_test15"
END_TEST_DATA
*/

struct X
{
	struct Y
	{
		Y()
		{
			value = 103;
		}
		
		int value = 0;
		int value2 = 1000;
	};
	
	struct Z
	{
		Z()
		{
			z = 90.0;
		}
		
	
		double z = 0.0;
	};
	
	Y y;
	Y y2;
	Z z;
};


int main(int input)
{
	X obj;
	
	return obj.y.value + obj.y2.value + (int)obj.z.z + obj.y.value2;
}
