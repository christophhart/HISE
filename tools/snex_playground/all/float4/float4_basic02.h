/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12.0f
  output: 19.0f
  error: ""
  filename: "float4/float4_basic02"
END_TEST_DATA
*/

float4 d = { 19.0f };

float main(float input)
{
	auto e = d;
	e[0] = 90.0f;	

	return d[0];
	
}

