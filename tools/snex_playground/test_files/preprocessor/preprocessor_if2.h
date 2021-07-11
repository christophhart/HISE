/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 92
  error: ""
  filename: "preprocessor/preprocessor_if2"
END_TEST_DATA
*/

#define ENABLE_FIRST 1


#if ENABLE_FIRST
#if true
int x = 92;
#else
int x = 12;
#endif
#else
int x = 190;
#endif

int main(int input)
{
	return x;
}

