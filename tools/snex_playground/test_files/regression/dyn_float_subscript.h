/*
BEGIN_TEST_DATA
  f: test
  ret: float
  args: float
  input: 12.0f
  output: 5.0f
  error: ""
  filename: "regression/dyn_float_subscript"
END_TEST_DATA
*/

span<float, 2> values = { 2.0f, 3.0f };
dyn<float> view;

float test(float input)
{
	view.referTo(values, values.size());
	return view[0] + view[1];
}
