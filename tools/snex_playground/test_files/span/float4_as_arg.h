/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/float4_as_arg"
END_TEST_DATA
*/

float4 data = { 1.0f, 2.0f, 3.0f, 4.0f };

void clearFloat4()
{
	data = 2.0f;
}

int main(int input)
{
	clearFloat4();
	
	return (int)data[3];
}

