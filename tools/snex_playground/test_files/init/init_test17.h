/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "init/init_test17"
END_TEST_DATA
*/

struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}

	void reset()
	{
		value *= 2;
	}
	

	int value = 8;
};

struct I
{
	I(X& x)
	{
		x.value = 12;
	}
};


wrap::init<X, I> obj;

int main(int input)
{
 
 	obj.reset();
  	
	return obj.obj.value;
	
	
}

