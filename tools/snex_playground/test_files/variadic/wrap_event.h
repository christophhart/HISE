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
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}

    int z = 12;
};

wrap::event<X> m;

int main(int input)
{
	return m.obj.z;
}

