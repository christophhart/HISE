/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/nested_struct_member_call"
END_TEST_DATA
*/

struct X 
{ 
    struct Y 
    { 
        int u = 8; 
        float v = 12.0f; 
        
        float getV() 
        { 
            return v; 
        }
    }; 
    Y y; 
}; 

X x; 

int main(int input)
{
	return 12;
	//return x.y.getV() + input;
}

