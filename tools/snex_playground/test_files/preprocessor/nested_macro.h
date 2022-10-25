/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
  error: ""
  filename: "preprocessor/nested_macro"
END_TEST_DATA
*/

#define MUL(a, b) a * b
#define ADD(a, b) a + b

int main(int input)
{
	return MUL((ADD(2, 3)), 4);
}

