/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 190
  error: ""
  filename: "preprocessor/preprocessor_if1"
END_TEST_DATA
*/


#if 0
int x = 12;
#else
int x = 190;
#endif

int main(int input)
{
	return x;
}

