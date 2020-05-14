/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "span/float4_as_arg"
END_TEST_DATA
*/

float4 data = { 1.0f, 2.0f, 3.0f, 4.0f };

void clearFloat4(float4& d)
{
	d = 2.0f;
}

int main(int input)
{
	clearFloat4(data);
	
	return (int)data[3];
}

