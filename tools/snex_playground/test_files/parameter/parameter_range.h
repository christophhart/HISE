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

struct ranges
{
	using T = double;

	static T from0To1(T min, T max, T value) { return Math.map(value, min, max); }
	static T to0To1(T min, T max, T value) { return (value - min) / (max - min); }
};



#define MIN_MAX(minValue, maxValue) static const double min = minValue; static const double max = maxValue;


#define RANGE_FUNCTION(id) static double id(double input) { return ranges::id(min, max, input); }

#define DECLARE_PARAMETER_RANGE(name, minValue, maxValue) struct name { MIN_MAX(minValue, maxValue)  \
RANGE_FUNCTION(to0To1); \
RANGE_FUNCTION(from0To1) \ }



DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);



struct Identity
{
	static double to0To1(double input) { return input; }
};

struct Test 
{
	template <int P> void setParameter(double v)
	{
		if(P == 0)
			value = v;
	}
	
	double value = 1.0;
};



using PType = parameter::from0To1<Test, 0, TestRange>;


container::chain<PType, Test, Test> c;



double main(double input)
{
	auto& first = c.get<0>();
	
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	
	c.setParameter<0>(0.5);
	
	return first.value;
}

