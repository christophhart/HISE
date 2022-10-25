/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/simple_reset"
END_TEST_DATA
*/

namespace funky
{

    struct X
    {
        DECLARE_NODE(X);

        template <int P> void setParameter(double v)
        {
          
        }

        void reset()
        {
            
        }
    };
}

container::chain<parameter::empty, wrap::fix<1, funky::X>, funky::X> c;

int main(int input)
{
	c.reset();
	return 12;
}

