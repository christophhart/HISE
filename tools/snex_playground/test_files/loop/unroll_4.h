/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 25
  error: ""
  loop_count: 0
  filename: "loop/unroll_4"
END_TEST_DATA
*/

struct X
{
    double unused = 2.0;
    int v = 5;
};

X x1;
X x2;

span<X, 5> d;

int main(int input)
{
	for(auto& s: d)
	    input += s.v;
	    
    return input;
}

