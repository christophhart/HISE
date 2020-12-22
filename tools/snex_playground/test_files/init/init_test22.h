/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "init/init_test22"
END_TEST_DATA
*/

struct e 
{
	DECLARE_NODE(e);
	static const int NumChannels = 1;

	template <int P> void setParameter(double v)
	{
		
	}

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



container::chain<parameter::empty, wrap::fix<1, wrap::init<wrap::event<e>, i>>> obj3;

int main(int input)
{
	return obj3.get<0>().getWrappedObject().v;
}
