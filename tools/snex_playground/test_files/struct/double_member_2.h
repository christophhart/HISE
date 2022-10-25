/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 7
  output: 12
  error: ""
  filename: "struct/double_member_2"
END_TEST_DATA
*/



struct X 
{ 
    struct Y 
    { 
        int value = 19; 
    }; 
    
    Y y; 
}; 

X x; 

int main(int input) 
{ 
    x.y.value = input + 5; 
    int v = x.y.value; 
    return v; 
}