/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 8.0f
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/simd_mul_scalar"
END_TEST_DATA
*/

span<float, 8> d = { 2.0f };

float main(float input)
{
    for(auto& v: d.toSimd())
    {
        v *= 0.5f;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

