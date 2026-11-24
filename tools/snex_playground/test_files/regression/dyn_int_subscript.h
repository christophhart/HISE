/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 12
  output: 28
  error: ""
  filename: "regression/dyn_int_subscript"
END_TEST_DATA
*/

span<int, 3> values = { 1, 2, 3 };
dyn<int> view;

int test(int input)
{
	view.referTo(values, values.size());
	view[1] = input;
	return view[0] + view[1] + view[2] + values[1];
}
