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


int main(int input)
{
	return sum<A, A, B>();
}

