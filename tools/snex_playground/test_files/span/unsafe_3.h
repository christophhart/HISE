/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 92
  error: ""
  filename: "span/unsafe_3"
END_TEST_DATA
*/

index::unsafe<9> i;

int main(int input)
{
    i = 91;
	return ++i;
}

