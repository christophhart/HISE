/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "preprocessor/constant_and_macro"
END_TEST_DATA
*/

#define VALUE 2
#define MUL(x) x * VALUE

int main(int input)
{
	return MUL(input);
}

