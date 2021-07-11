/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 23(9): private struct X::Inner is not accessible"
  filename: "visibility/private_struct"
END_TEST_DATA
*/


class X
{
	struct Inner
	{
		int x = 5;
	};
};


X::Inner obj;

int main(int input)
{
	return obj.x;
}

