/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 0.5
  output: 12
  error: ""
  filename: "funky"
END_TEST_DATA
*/


namespace ranges
{
	

struct RangeBase
{
	using T = double;

	static T from0To1(T min, T max, T value) { return Math.map(value, min, max); }
	static T to0To1(T min, T max, T value) { return (value - min) / (max - min); }
};

	
}

#define MIN_MAX(minValue, maxValue) static const double min = minValue; static const double max = maxValue;


#define RANGE_FUNCTION(id) static double id(double input) { return ranges::RangeBase::id(min, max, input); }

#define DECLARE_PARAMETER_RANGE(name, minValue, maxValue) struct name { MIN_MAX(minValue, maxValue)  RANGE_FUNCTION(to0To1); RANGE_FUNCTION(from0To1) };

DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0)

struct Test
{
	template <int P> void setParameter(double v)
	{
		if(P)
			value = v;
	}
	
	double value = 12.0;
};

Test t;




double main(double input)
{
	
	
	//t.setParameter<0>(19.0);	
	
	return t.value;
}

