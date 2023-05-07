/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 48.0f
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/simd_set_scalar"
END_TEST_DATA
*/

span<float, 12> d = { 2.0f };



float main(float input)
{
    for(auto& v: d.toSimd())
    {
        float x = 4.0f;
        v = x;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}
