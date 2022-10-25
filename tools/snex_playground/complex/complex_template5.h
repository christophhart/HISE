/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 35
  error: ""
  filename: "template/complex_template5"
END_TEST_DATA
*/

span<int, 5> data = { 1, 2, 3, 4, 5 };

span<int, 3> bigData = { 10, 20, 30 };

template <int C, typename T> T getLast(span<T, C>& d)
{
    return d[C-1];
};

int main(int input)
{
	return getLast(data) + getLast(bigData);
}

