/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "span/unsafe_1"
END_TEST_DATA
*/

index::unsafe<9> i = { 12 };

int main(int input)
{
	return i;
}

