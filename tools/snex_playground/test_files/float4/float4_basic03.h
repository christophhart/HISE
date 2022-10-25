/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12.0f
  output: 90.0f
  error: ""
  filename: "float4/float4_basic03"
END_TEST_DATA
*/

span<float, 4> d = { 19.0f };

float main(float input)
{
	d[0] = 90.0f;	

	return d[0];	
}

