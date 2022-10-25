/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "init/init_test6"
END_TEST_DATA
*/

struct X
{
	X(int a, float z, double b)
	{
		v1 = a;
		v2 = z;
		v3 = b;
	}
	
	int v1 = 0;
	float v2 = 0.0f;
	double v3 = 0.0;
};

X obj = { 1, 2.0f, 3.0};


int main(int input)
{
	return (int)obj.v3 + (int)obj.v2 + obj.v1;
}

