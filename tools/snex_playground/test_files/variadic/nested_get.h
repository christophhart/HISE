/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "variadic/nested_get"
END_TEST_DATA
*/

struct X
{
    int v = 5;
    double x = 2.0;
    span<float, 4> data = { 2.0f, 0.5f, 1.0f, 9.2f };
};

container::chain<X, container::chain<X, X>> c;

int main(int input)
{
	return (int)c.get<1>().get<1>().data[3];
}

