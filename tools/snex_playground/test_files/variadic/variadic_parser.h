/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/variadic_parser"
END_TEST_DATA
*/

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double d)
    {
      
    }

    static const int NumChannels = 1;

    int v = 0;
    
    
};

container::chain<parameter::empty, wrap::fix<1, X>, container::chain<parameter::empty, X, X>> c;

int main(int input)
{
    return 12;
}

