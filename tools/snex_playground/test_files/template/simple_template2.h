/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "template/simple_template2"
END_TEST_DATA
*/

template <typename T> struct X
{
    T x = (T)2;
};

X<int> c;
X<float> f;

int main(int input)
{
	return c.x + (int)f.x;
}

