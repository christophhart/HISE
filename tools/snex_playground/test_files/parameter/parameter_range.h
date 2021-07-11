/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 0.5
  output: 110
  error: ""
  filename: "parameter/parameter_range"
END_TEST_DATA
*/





DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);



struct Identity
{
	static double to0To1(double input) { return input; }
};

struct Test 
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		if(P == 0)
			value = v;
	}
	
	double value = 1.0;
};



using PType = parameter::from0To1<Test, 0, TestRange>;


container::chain<PType, wrap::fix<1, Test>, Test> c;



double main(double input)
{
	auto& first = c.get<0>();
	
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	
	c.setParameter<0>(0.5);
	
	return first.value;
}

