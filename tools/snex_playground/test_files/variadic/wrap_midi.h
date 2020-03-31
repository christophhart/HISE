/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/wrap_midi"
END_TEST_DATA
*/

struct X
{
    int z = 12;
};

wrap::midi<X> m;

int main(int input)
{
	return m.obj.z;
}

