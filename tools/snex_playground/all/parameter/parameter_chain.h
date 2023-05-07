/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 4.0
  error: ""
  filename: "parameter/parameter_chain"
END_TEST_DATA
*/

struct Identity
{
	static double to0To1(double v)
	{
		return v;
	}
};

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
using ParameterChainType = parameter::chain<Identity, ParameterType, ParameterType>;

ParameterChainType pc;

container::chain<ParameterChainType, wrap::fix<1, Test>, Test> c;

void op()
{
	pc.call(2.0);
}

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	pc.connect<0>(first);
	pc.connect<1>(second);

	op();
	
	return c.get<0>().value + c.get<1>().value;
}


