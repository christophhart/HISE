/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "init/init_test21"
END_TEST_DATA
*/

struct e 
{
	DECLARE_NODE(e);

	template <int P> void setParameter(double v) {}

	void reset()
	{
		
	}
	
	int v = 90;
};


struct i
{
	i(wrap::event<e>& o)
	{
		o.getObject().v = 18;
	}
};

wrap::init<wrap::event<e>, i> obj;

int main(int input)
{
	return obj.getWrappedObject().v;
}

