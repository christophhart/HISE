/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "basic/span_iteration"
END_TEST_DATA
*/

span<float, 16> c1 = { 4.0f };

int main(int input)
{
    for(auto& s: c1)
    {
        s *= 0.5f;
    }
    
    return (int)c1[0];
}

