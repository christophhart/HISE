/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "wrap/index_type"
END_TEST_DATA
*/

span<float, 5> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

dyn<float> b;

int main(int input)
{
	b = data;
	

	auto d = IndexType::wrapped(b);
	
	
	return (int)b[d.moved(9)];
}

