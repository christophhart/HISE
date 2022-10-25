/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 0
  error: ""
  filename: "loop/simdable_non_aligned"
END_TEST_DATA
*/

span<float, 16> s = { 2.0f };
dyn<float> d;

int main(int input)
{
	d.referTo(s, 2, 8);
	
	return d.isSimdable();
}

