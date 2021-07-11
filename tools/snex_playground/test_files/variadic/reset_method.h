/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "variadic/reset_method"
END_TEST_DATA
*/

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    static const int NumChannels = 2;

    void reset()
    {
        v = 1;
    }
    
    int v = 8;
};

container::chain<parameter::empty, X, X, X> c;

int main(int input)
{
	c.reset();
	
	return c.get<1>().v + c.get<0>().v;
}

