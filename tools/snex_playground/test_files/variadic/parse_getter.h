/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "variadic/parse_getter"
END_TEST_DATA
*/

struct X
{
    int v = 8;
};

container::chain<X, X> c;

int main(int input)
{
	c.get<1>().v = input * 2;
	
	return c.get<1>().v;
}

