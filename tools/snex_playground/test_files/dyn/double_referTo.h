/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "dyn/double_referTo"
END_TEST_DATA
*/

span<float, 12> data = { 90.0f };

dyn<float> d1;
dyn<float> d2;

void ref1()
{
	d1.referTo(data, data.size());
}

int main(int input)
{
	ref1();
	d2.referTo(d1, d1.size());
	
	return d2[0];
}

