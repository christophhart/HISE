/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "index/index7"
END_TEST_DATA
*/

index::wrapped<32, false> j;

int main(int input)
{
  	return (int)++j;
}

