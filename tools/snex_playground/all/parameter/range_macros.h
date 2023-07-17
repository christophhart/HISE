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

DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);


double main(double input)
{
	return TestRange::from0To1(0.5);
}

