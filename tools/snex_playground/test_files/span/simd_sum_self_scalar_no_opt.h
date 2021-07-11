/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 164.0f
  error: ""
  compile_flags: AutoVectorisation
  filename: "span/simd_sum_self_scalar_no_opt"
END_TEST_DATA
*/

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float sum = 0.0f;
    
    float x = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        // this should add 9.0f to the first half
        // and 17.0f to the second half
        s += s[1] + s[0];
    }
    
    for(auto& s: d)
        sum += s;
    
    // float expected = 60.0f + 4.0f * 9.0f + 4.0f * 17.0f;
        
    return sum;
}

