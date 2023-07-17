/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 82.0
  error: ""
  filename: "parameter/plain_parameter"
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

double main(double input)
{
	Test obj;
	ParameterType pType;
	pType.connect<0>(obj);
	pType.call(80.0);

	
	auto& first = c.get<0>();
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	c.setParameter<0>(2.0);

	return c.get<0>().value + obj.value;

}


