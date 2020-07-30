/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 5.0f
  error: ""
  filename: "struct/local_struct_with_default"
END_TEST_DATA
*/


using T = float;

struct X
{
    T v1 = 2.0f;
    T v2 = 3.f;
    T v3 = 0.f;
};



T main(T input)
{
    X v;
    
    return v.v1 + v.v2;
}

