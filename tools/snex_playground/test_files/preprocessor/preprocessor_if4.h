/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 10
  error: ""
  filename: "preprocessor/preprocessor_if4"
END_TEST_DATA
*/

#define ENABLE_FIRST 0


int main(int input)
{
    #if ENABLE_FIRST
	return x;
	#else
	return 10;
	#endif
}

