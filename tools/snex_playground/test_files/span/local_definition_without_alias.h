/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 5.0f
  error: ""
  filename: "span/local_definition_without_alias"
END_TEST_DATA
*/


float main(float input)
{
    span<float, 2> data = { 2.0f, 3.0f };
    
    return data[0] + data[1];
}

