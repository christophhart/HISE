/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "default_arg/IndexTypeDefault"
END_TEST_DATA
*/


span<int, 10> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };


int main(int input)
{
  auto firstIndex = IndexType::wrapped(data);

	auto first = data[firstIndex];
	
  auto secondIndex = IndexType::wrapped(data, input);

	auto second = data[secondIndex];
	
	return first + second;
	
}

