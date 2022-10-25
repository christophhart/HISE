/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/local_span_ref"
END_TEST_DATA
*/



int main(int input)
{
	span<int, 9> d = { 80};

	auto& x = d[2];
	
	d[2] = 9;
	
	return d[2];
}

