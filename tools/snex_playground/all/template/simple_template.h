/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/simple_template"
END_TEST_DATA
*/

template <typename T> struct X
{
    T x = (T)2;
};

X<int> c;

int main(int input)
{
	return c.x;
}

