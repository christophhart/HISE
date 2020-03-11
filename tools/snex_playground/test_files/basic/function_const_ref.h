/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 22: Can't modify const object"
  filename: "basic/function_const_ref"
END_TEST_DATA
*/



struct X
{
    int v = 12;
};

void resetObject(const X& copy)
{
    copy.v = 0;
}

X x;

int main(int input)
{
	resetObject(x);
	
	return x.v;
}

