/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "span/loop_break_continue_nested1"
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
            if(s == 3.0f)
                break;

            sum += s;
        }
    }
    
    return (int)sum;
}

