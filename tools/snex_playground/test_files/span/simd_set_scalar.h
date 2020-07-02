/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 32.0f
  error: ""
  filename: "span/simd_set_scalar"
END_TEST_DATA
*/

span<float, 8> d = { 2.0f };

float4 x = { 1.0f, 2.0f, 3.0f, 4.0f };

float main(float input)
{
    for(auto& v: d.toSimd())
    {
        v = 4.0f;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

