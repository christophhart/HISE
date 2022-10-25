/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 1.5
  error: ""
  filename: "span/add_float4_float5"
END_TEST_DATA
*/



float main(float input)
{
    span<float, 5> d5 = { 1.0f };
    span<float, 4> d4 = { 0.5f };
    
    
	return d4[2] + d5[1];
}

