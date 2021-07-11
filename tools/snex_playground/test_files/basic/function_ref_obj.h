/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "basic/function_ref_obj"
END_TEST_DATA
*/

bool handleModulation(int& v)
{
	v = 90;
	return true;
}

struct X
{
	bool handleModulation(int& v)
	{
		v = 90.0;
		return true;
	}
	
};

X obj;
int x = 8;

int main(int input)
{
	obj.handleModulation(x);
	return (int)x;
}

