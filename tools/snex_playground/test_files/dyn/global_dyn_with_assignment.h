/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 16
  error: ""
  filename: "dyn/global_dyn_with_assignment"
END_TEST_DATA
*/

span<int, 8> data = { 2 };

dyn<int> d;

int main(int input)
{
    d.referTo(data, data.size());
	
    for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

