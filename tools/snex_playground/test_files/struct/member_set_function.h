/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "struct/member_set_function"
END_TEST_DATA
*/

struct X
{
  int v = 1;

  void setV(int value)
  {
    v = value;
  }
};

X x;

int main(int input)
{
	x.setV(90);
  return x.v;
}

