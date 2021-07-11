/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "basics/return_reference"
END_TEST_DATA
*/

span<int, 4> data = { 1, 2, 3, 4 };

int& getSecond()
{
	return data[1];
}

int main(int input)
{
	return getSecond();
}

