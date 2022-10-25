/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "index/index8"
END_TEST_DATA
*/

index::unsafe<32, false> j;

span<int, 32> data = { 9 };

int main(int input)
{
	return data[j];
	
}

