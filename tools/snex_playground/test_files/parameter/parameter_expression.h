/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 9
  error: ""
  filename: "parameter/parameter_expression"
END_TEST_DATA
*/



DECLARE_PARAMETER_EXPRESSION(TestExpression, 2.0 * input - 1.0);


struct Test 
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 2.0;
};

using ParameterType = parameter::expression<Test, 0, TestExpression>;

container::chain<ParameterType, wrap::fix<1, Test>, Test> c;

ParameterType p;


double x = 5.0;

double main(double in)
{
	auto& second = c.get<1>();
	p.connect<0>(second);
	
	p.call(x);
	
	return c.get<1>().value;
}

