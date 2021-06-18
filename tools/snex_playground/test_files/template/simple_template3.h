/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "template/simple_template3"
END_TEST_DATA
*/

template <typename T> struct X
{
    T getSquare()
    {
        return x * x;
    }
    
    T x = (T)2;
};

X<int> c;

int main(int input)
{
	return c.getSquare();
}

