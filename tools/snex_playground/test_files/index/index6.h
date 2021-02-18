/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "index/index6"
END_TEST_DATA
*/

index::normalised<float, index::wrapped<0, false>> i;

span<float, 4> data = { 1.0f, 4.0f, 9.0f, 12.0f };

int main(int input)
{
	i = 0.5f;

	return data[i];
}

