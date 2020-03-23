/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 80
  error: ""
  filename: "dyn/dyn_wrap_1"
END_TEST_DATA
*/

span<int, 6> s = { 1, 2, 3, 4, 5, 6 };
dyn<int> d;

dyn<int>::wrapped i;


int main(int input)
{
    i = 80;
	return i;
}

