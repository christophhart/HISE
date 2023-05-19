/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 6
  output: 4
  error: ""
  filename: "dyn/dyn_access2"
END_TEST_DATA
*/

span<int, 10> data = { 4 };
dyn<int> b;

int main(int input)
{
	b.referTo(data, data.size());
	
	return b[29 % input];
}

