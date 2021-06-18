/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "struct/struct_ref2function"
END_TEST_DATA
*/

struct X
{
    float z = 129.0f;
};

void change(X& obj)
{
    obj.z = 90.0f;
}

X x;

int main(int input)
{
	change(x);
	
	return (int)x.z;
}

