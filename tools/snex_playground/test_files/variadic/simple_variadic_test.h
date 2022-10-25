/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/simple_variadic_test"
END_TEST_DATA
*/

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    int v = 0;
    
    void process(int input)
    {
        v = input;
    }
};

container::chain<parameter::empty, wrap::fix<1, X>, X> c;

int main(int input)
{
    return 12;
}