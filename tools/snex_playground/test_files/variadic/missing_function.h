/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 29(9): X does not have a method reset"
  filename: "variadic/missing_function"
END_TEST_DATA
*/

struct X
{ 
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    int v = 9;
};

container::chain<parameter::empty, container::chain<parameter::empty, wrap::fix<1, X>, X>, X> c;

int main(int input)
{
	c.reset();
    
    return c.get<0>().get<1>().v;
}

