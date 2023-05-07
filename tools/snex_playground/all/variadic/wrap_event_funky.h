/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "variadic/wrap_midi_funky"
END_TEST_DATA
*/

struct X
{
  DECLARE_NODE(X);

  template <int P> void setParameter(double v) {}
  
    void reset()
    {
        x = 90;
    }
    
    int x = 12;
};

wrap::event<X> m;

int main(int input)
{
  m.reset();
	return m.obj.x;
}

