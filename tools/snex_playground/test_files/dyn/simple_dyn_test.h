/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 16
  error: ""
  filename: "dyn/simple_dyn_test"
END_TEST_DATA
*/

span<int, 8> data = { 2 };

int main(int input)
{
    dyn<int> d;

    d.referTo(data, data.size());
	
    for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

