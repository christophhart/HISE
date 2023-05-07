/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "span/loop_nested"
END_TEST_DATA
*/

span<span<float, 3>, 5> d = { {1.0f, 2.0f, 3.0f}};

int main(int input)
{
    float sum = 0.0f;
    
	  for(auto& i: d)
    {
        for(auto& s: i)
        {
            sum += s;
        }
    }
    
    return (int)sum;
}

