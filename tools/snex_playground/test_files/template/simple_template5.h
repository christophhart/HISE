/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 36
  error: ""
  filename: "template/simple_template5"
END_TEST_DATA
*/

template <typename T> struct X
{
    T multiply(T v)
    {
        return x * v;
    }
    
    T x = (T)2;
};

X<int> c;
X<int> c2;

int main(int input)
{
    c2.x = 1;
    
	return c.multiply(input) + c2.multiply(input);
}

