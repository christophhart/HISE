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
    DECLARE_NODE(X);
    static const int NumChannels = 1;

    template <int P> void setParameter(double v)
    {
      
    }
    
    int v = 8;
};

container::chain<parameter::empty, X, X> c;

int main(int input)
{
	c.get<1>().v = input * 2;
	
	return c.get<1>().v;
}

