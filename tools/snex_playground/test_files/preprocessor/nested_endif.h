/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "preprocessor/nested_endif"
END_TEST_DATA
*/


#if 0
#define X 125125

#if 0
#define Y asags
#endif

MUST NOT BE COMPILED!!!%$$

#endif

int main(int input)
{
	return 12;
}

