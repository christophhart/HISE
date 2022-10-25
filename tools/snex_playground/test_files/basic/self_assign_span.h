/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/self_assign_span"
END_TEST_DATA
*/

void p(span<float, 2>& d)
{
	auto& s = d[0];
	//s = s * 2.0f;
	d[0] = s;
}

int main(int input)
{
	span<float, 2> x = { 12.0f};

	p(x);

	return (int)x[0];

}

