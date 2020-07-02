/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 9.0f
  error: ""
  filename: "struct/simd_on_member_span"
END_TEST_DATA
*/


using T = float;

struct X
{
    span<float, 8> s;
};


X x2 = { { 7.0f } };

T main(T input)
{
    for(auto& v: x2.s.toSimd())
    {
        v = 9.0f;
    }
    
    return x2.s[0];
}

