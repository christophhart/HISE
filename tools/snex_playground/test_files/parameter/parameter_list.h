/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 7
  error: ""
  filename: "parameter/parameter_list"
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
using PList = parameter::list<ParameterType, ParameterType>;

container::chain<PList, wrap::fix<1, Test>, Test> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	auto& p1 = c.getParameter<0>();
	auto& p2 = c.getParameter<1>();

	p1.connect<0>(first);
	p2.connect<0>(second);

	c.setParameter<0>(4.0);
	c.setParameter<1>(3.0);

	return c.get<0>().value + c.get<1>().value;
}


