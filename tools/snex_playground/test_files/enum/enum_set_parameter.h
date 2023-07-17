/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12.0
  output: 72.0
  error: ""
  filename: "enum/enum_set_parameter"
END_TEST_DATA
*/

struct processor
{
	enum Parameter
	{
		FirstParameter,
		SecondParameter,
		numParameters
	};
	
	template <int P> void setParameter(double value)
	{
		v[P] = value * 2.0;
	}
	
	span<double, Parameter::numParameters> v;
};

processor p;
//processor<67> p2;

void change(double input)
{
	p.setParameter<1>(input);
	p.setParameter<0>(input * 2.0);	
}

double main(double input)
{
	change(input);
	
	
	return p.v[1] + p.v[0];
}

