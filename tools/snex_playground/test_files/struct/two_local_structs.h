/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 4.0f
  error: ""
  filename: "struct/two_local_structs"
END_TEST_DATA
*/


using T = float;

struct X
{
    T v1 = 0.0f;
    T v2 = 0.f;
    T v3 = 0.f;
};



T main(T input)
{
    X x1 = { 2.0f, 3.0f, 4.0f};
    X x2 = { 1.0f };
    
    return x1.v2 + x2.v1;
}

