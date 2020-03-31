/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/span_in_template"
END_TEST_DATA
*/


template <int NumElements, typename T> struct X
{
    span<T, NumElements> x = { (T)2 };
};

X<4, int> c;


int main(int input)
{
	return c.x[0];
}

