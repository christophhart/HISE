/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 2.0
  error: ""
  filename: "parameter/plain_parameter"
END_TEST_DATA
*/

struct Test
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;

container::chain<ParameterType, Test, Test> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	c.setParameter<0>(2.0);

	return c.get<0>().value;
}


