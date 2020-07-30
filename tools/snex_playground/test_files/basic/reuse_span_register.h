/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 13.0f
  error: ""
  filename: "basic/reuse_span_register"
END_TEST_DATA
*/

span<float, 5> data = { 2.0f };



float main(float input)
{
    data[0] = 12.0f;
    data[0] = 13.0f;
    
	  return data[0];
}