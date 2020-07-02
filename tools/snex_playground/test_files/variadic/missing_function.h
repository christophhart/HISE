/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 22: X does not have a method reset"
  filename: "variadic/missing_function"
END_TEST_DATA
*/

struct X
{
    int v = 9;
};

container::chain<container::chain<X, X>, X> c;

int main(int input)
{
    c.reset();
    
	  return c.get<0>().get<1>().v;
}

