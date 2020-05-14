/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 30.0f
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/local_definition_scalar"
END_TEST_DATA
*/

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float4 v = { 0.5f };
    
    float sum = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        s *= v;
    }
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

