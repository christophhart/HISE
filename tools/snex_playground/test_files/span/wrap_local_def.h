/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "span/wrap_local_def"
END_TEST_DATA
*/



int main(int input)
{
	index::wrapped<8> i;
	
	return (int)--i;
}

