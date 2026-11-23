/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "regression/dyn_refer_to"
END_TEST_DATA
*/
span<float, 4> data = { 0.0f };
dyn<float> view;

int test(int value)
{
	view.referTo(data, data.size());
	for (auto& sample : view)
		sample = (float)value;
	return (int)data[0];
}
