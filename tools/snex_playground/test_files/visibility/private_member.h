/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 24(13): private int X::x is not accessible"
  filename: "visibility/private_member"
END_TEST_DATA
*/

struct X
{
private:

	int x = 12;
};

X obj;

int main(int input)
{
	return obj.x;	
}

