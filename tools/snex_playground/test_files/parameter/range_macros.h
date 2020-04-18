/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 0.5
  output: 110
  error: ""
  filename: "parameter/range_macros"
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

#define DECLARE_PARAMETER_RANGE(name, minValue, maxValue) struct name \
{ MIN_MAX(minValue, maxValue)  \
RANGE_FUNCTION(to0To1); \
RANGE_FUNCTION(from0To1) \
 }

DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);


double main(double input)
{
	return TestRange::from0To1(0.5);
}

