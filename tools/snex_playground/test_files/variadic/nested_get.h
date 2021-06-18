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
    DECLARE_NODE(X);

    static const int NumChannels = 2;

    template <int P> void setParameter(double v)
    {
      
    }

    int v = 5;
    double x = 2.0;
    span<float, 4> data = { 2.0f, 0.5f, 1.0f, 9.2f };
};



container::chain<parameter::empty, wrap::fix<2, X>, container::chain<parameter::empty, X, X>> c;

int main(int input)
{
	return (int)c.get<1>().get<1>().data[3];
}

