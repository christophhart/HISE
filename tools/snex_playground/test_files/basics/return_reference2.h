/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "basics/return_reference2"
END_TEST_DATA
*/

span<int, 4> data = { 1, 2, 3, 4 };

int& getWrapped()
{
	return data[1];
}

int& getSecond()
{
	return getWrapped();
}

int main(int input)
{
	auto& x = getSecond();

	return x;
}

