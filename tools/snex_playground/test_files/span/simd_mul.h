/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 8.0f
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/simd_mul"
END_TEST_DATA
*/

span<float, 8> d = { 2.0f };

float4 x = { 0.5f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& v: d.toSimd())
    {
        v *= x;
    }
    
    for(auto& v: d)
        sum += v;
    
    return sum;
}

