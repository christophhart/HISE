/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "dyn/size_test"
END_TEST_DATA
*/

span<float, 19> d;

dyn<float> data;

int main(int input)
{
	data = d;
	
	return data.size();
}

