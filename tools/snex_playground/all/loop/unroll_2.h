/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 162
  error: ""
  loop_count: 0
  filename: "loop/unroll_2"
END_TEST_DATA
*/

struct X
{
    int v1 = 12;
    float v2 = 18.0f;
    
    float sum()
    {
        return v2 + (float)v1;
    }
};

span<X, 5> d;

int main(int input)
{
    for(auto& s: d)
    {
        input += (int)s.sum();
    }
    
    return input;
}

