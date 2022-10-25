/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 11
  error: ""
  filename: "template/simple_template10"
END_TEST_DATA
*/

template <typename T> struct X
{
    T obj;
};

X<float> f = { 8.0f };

X<span<int, 3>> s = { {1, 2, 3} };

int main(int input)
{
	return (int)f.obj + s.obj[0] + s.obj[1];
}

