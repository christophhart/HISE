/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 21
  error: ""
  filename: "basic/simple struct"
END_TEST_DATA
*/

struct MyObject
{
    int x = 9;
    int y = 12;
};

MyObject c;

int main(int input)
{
	return c.x + c.y;
}

