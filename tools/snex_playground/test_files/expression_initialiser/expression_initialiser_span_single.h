/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 26
  error: ""
  filename: "expression_initialiser/expression_initialiser_span_single"
END_TEST_DATA
*/

int v = 12;

int main(int input)
{
	span<int, 2> x = { ++v };
	
	return x[0] + x[1];
}

