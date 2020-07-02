/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "namespace/namespaced_struct"
END_TEST_DATA
*/

namespace MyObjects
{
    struct Obj
    {
        int v = 15;
    };
}


MyObjects::Obj c;

int main(int input)
{
    MyObjects::Obj d;
    
	return c.v + d.v;
}

