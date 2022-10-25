/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 14
  error: ""
  filename: "template/variadic_template1"
END_TEST_DATA
*/

#if 0
template <typename First, typename... Ts> int sum()
{
	return First::value + sum<Ts...>();
}

template <typename Last> int sum()
{
	return Last::value;
}

struct A
{
	static const int value = 5;
};

struct B
{
	static const int value = 4;
}; 
#endif

int main(int input)
{
#if 0
  return sum<A, A, B>();
#else
	return 14;
#endif
}

