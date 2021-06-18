/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 5.0f
  error: "Line 27(8): initialiser mismatch: { 2.f, 3.f, 4.f, 5.f } (expected 3)"
  filename: "struct/local_struct_definition_wrong"
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
    X v = { 2.0f, 3.0f, 4.0f, 5.0f };
    
    return v.v1 + v.v2;
}

