/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 42
  error: ""
  filename: "loop/unroll_5"
END_TEST_DATA
*/

struct X
{
    int v1 = 12;
    float v2 = 18.0f;
};

span<X, 5> d;

int main(int input)
{
    auto& s = d[0];
    input += (int)(s.v2 + (float)s.v1);
    
    return input;
}

