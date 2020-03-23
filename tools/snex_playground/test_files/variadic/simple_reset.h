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
        void reset()
        {
            
        }
    };
}

container::chain<funky::X, funky::X> c;

int main(int input)
{
	c.reset();
	return 12;
}

