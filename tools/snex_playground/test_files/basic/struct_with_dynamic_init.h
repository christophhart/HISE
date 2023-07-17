/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 5
  output: 25
  error: ""
  filename: "basic/struct_with_dynamic_init"
END_TEST_DATA
*/

struct X
{
	int x = 9;
	int y = 8;
	double z = 2.0;
};

int main(int input)
{
	X obj = { 10, input * 2, (double)input};
	
	return obj.y + obj.x + (int)obj.z;
}

