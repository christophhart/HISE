/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 27
  error: ""
  loop_count: 0
  filename: "loop/unroll_1"
END_TEST_DATA
*/

span<int, 5> d = { 1, 2, 3, 4, 5 };

int main(int input)
{
	for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

