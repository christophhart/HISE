/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 7
  output: 7
  error: ""
  filename: "regression/dyn_refer_to_dyn"
END_TEST_DATA
*/
span<float, 4> data = { 0.0f };
dyn<float> source;
dyn<float> target;

int test(int value)
{
	source.referTo(data, data.size());
	target.referTo(source, 2, 1);
	target[0] = (float)value;
	return (int)data[1];
}
