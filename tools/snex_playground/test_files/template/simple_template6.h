/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "template/simple_template6"
END_TEST_DATA
*/

template <int MaxSize=90> struct X
{
    int getX()
    {
      return x * MaxSize;
    }

    int x = 2;
};

X<5> c;
X<8> c2;

int main(int input)
{
    c2.x = 1;
    
	  return c.getX() + c2.getX();
}

