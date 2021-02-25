/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 100
  error: ""
  filename: "span/unsafe_4"
END_TEST_DATA
*/

index::unsafe<9> i;

int main(int input)
{
    i = 91;
    i = (int)i+9;
	return i;
}

