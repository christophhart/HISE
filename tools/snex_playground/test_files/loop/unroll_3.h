/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 22
  error: ""
  loop_count: 0
  filename: "loop/unroll_3"
END_TEST_DATA
*/

span<span<int, 2>, 2> d = { {1, 2}, {3, 4} };

int main(int input)
{
	for(auto& l1: d)
    {
        for(auto& l2: l1)
        {
            input += l2;
        }
    }
    
    return input;
}

