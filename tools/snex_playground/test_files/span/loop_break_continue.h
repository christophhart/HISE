/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "span/loop_break_continue"
END_TEST_DATA
*/

span<int, 5> d = { 1, 2, 3, 4, 5 };



int main(int input)
{
    int sum = 0;
    
	for(auto& s: d)
    {
        if(s == 3)
            continue;
        
        if(s == 5)
            break;
            
        sum += s;
    }
    
    return sum;
}

