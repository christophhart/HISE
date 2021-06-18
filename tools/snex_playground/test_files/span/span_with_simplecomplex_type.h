/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "span/span_with_simplecomplex_type"
END_TEST_DATA
*/



struct X
{
    int v = 12;
};

using MySpan = span<X, 2>;

MySpan x;

int main(int input)
{
    return x[0].v;
}

