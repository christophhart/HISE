/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "template/template_static_function"
END_TEST_DATA
*/


struct C
{
  static int get() { return 9; }
};

template <typename T> int callSomething()
{
  return T::get();
}




int main(int input)
{
	return callSomething<C>();
}

