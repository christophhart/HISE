/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 180
  error: ""
  filename: "wrap/wrap_init1"
END_TEST_DATA
*/

struct X
{
	DECLARE_NODE(X);
	
	template <int P> void setParameter(double v) {};

	X()
	{
		
	}
	
	int value = 90;
};

struct In
{
	In(X& o)
	{
		o.value = 180;
	}
};

wrap::init<X, In> obj;

int main(int input)
{
	return obj.getWrappedObject().value;
}

