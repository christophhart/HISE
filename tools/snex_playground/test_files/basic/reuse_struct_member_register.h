/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/reuse_struct_member_register"
END_TEST_DATA
*/

struct X
{
    int v = 0;
};

X x;

int main(int input)
{
    x.v = 13;
	x.v = 12;
	return x.v;
}

