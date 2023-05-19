/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "dyn/dyn_access1"
END_TEST_DATA
*/

span<int, 4> data = { 1, 2, 3, 4 };
dyn<int> b;

int main(int input)
{
	b.referTo(data, data.size());
	
	return b[1];
}

