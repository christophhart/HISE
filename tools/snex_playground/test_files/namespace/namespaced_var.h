/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "namespace/namespaced_var"
END_TEST_DATA
*/


namespace Space
{
    int x = 5;
}

int main(int input)
{
	return Space::x;
}

