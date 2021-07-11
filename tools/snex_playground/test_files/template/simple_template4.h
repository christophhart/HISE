/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "template/simple_template4"
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

int main(int input)
{
	return c.multiply(input);
}

