/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 2.0
  output: 2.0
  error: ""
  filename: "parameter/plain_parameter_in_chain"
END_TEST_DATA
*/

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;

container::chain<ParameterType, wrap::fix<1, Test>, Test> c;

void op(double input)
{
	c.setParameter<0>(input);	
}

double main(double input)
{
	auto& first = c.get<1>();
	
	auto& p1 = c.getParameter<0>();

	p1.connect<0>(first);

	op(input);
	
	
	return c.get<1>().value;
}


