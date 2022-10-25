/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "struct/nested_member_call"
END_TEST_DATA
*/


struct X
{
    int x = 12;
    
    int first()
    {
        return x;
    }
    
    int second()
    {
        return first();
    }
};

X obj;

int main(int input)
{
	return obj.second();
}

