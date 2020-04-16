/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 20(1): Can't assign to target"
  filename: "basic/const_function"
END_TEST_DATA
*/

struct X
{
	int v = 12;
	
	void tryToChangeX() const
	{
		v = 18;
	}
};

X obj;


int main(int input)
{
	obj.tryToChangeX();
	
	return obj.v;
}

