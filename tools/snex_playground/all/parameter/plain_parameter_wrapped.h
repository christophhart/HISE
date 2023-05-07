/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 6
  error: ""
  filename: "parameter/plain_parameter_wrapped"
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

using WrappedTest = wrap::event<Test>;

using ParameterType = parameter::plain<Test, 0>;
using WrappedParameterType = parameter::plain<WrappedTest, 0>;

container::chain<WrappedParameterType, wrap::fix<1, WrappedTest>> c2;
container::chain<ParameterType, wrap::fix<1, WrappedTest>, WrappedTest> c;

double main(double input)
{
	c.getParameter<0>().connect<0>(c.get<0>());
	c.setParameter<0>(2.0);

	c2.getParameter<0>().connect<0>(c2.get<0>());
	c2.setParameter<0>(4.0);


	return c.get<0>().value + c2.get<0>().value;

}


