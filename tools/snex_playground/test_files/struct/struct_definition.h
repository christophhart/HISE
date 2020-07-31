/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 5.0f
  error: ""
  filename: "struct/struct_definition"
END_TEST_DATA
*/

struct X
{
    float first = 1.0f;
    float second = 0.0f;
};

X v = { 2.0f, 3.0f };

float main(float input)
{
    
    
    
    return v.first + v.second;
}

