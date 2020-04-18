/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 29
  error: ""
  filename: "parameter/parameter_mixed"
END_TEST_DATA
*/

struct OtherTest
{
	template <int P> void setParameter(double v)
	{
		if(P == 1)
			o = v * 9.0;
	}
	
	double o = 12.0;
};

struct Test
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};


#define DECLARE_PARAMETER_EXPRESSION(name, expression) struct name { static double op(double input) { return expression; }}

DECLARE_PARAMETER_EXPRESSION(TestExpression,  input + 1.0);


using ParameterType1 = parameter::plain<Test, 0>;
using OtherParameter = parameter::expression<OtherTest, 1, TestExpression>;

parameter::chain<ParameterType1, OtherParameter> pc;

container::chain<Test, OtherTest> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	pc.get<0>().connect(first);
	pc.get<1>().connect(second);

	pc.call<0>(2.0);
	return c.get<0>().value + c.get<1>().o;
}


