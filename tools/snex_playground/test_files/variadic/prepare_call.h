/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 512
  error: ""
  filename: "variadic/prepare_call"
END_TEST_DATA
*/

struct X
{
    int x = 0;
    
    void prepare(PrepareSpecs ps)
    {
        x = ps.blockSize;
        ps.blockSize = 100;
    }
};

struct Y
{
    int value = 0;
    
    void prepare(PrepareSpecs ps)
    {
        value = ps.blockSize * 2;
    }
};

container::chain<X, Y> c;


int main(int input)
{
	PrepareSpecs ps = { 44100.0, 512, 2 };
	
	c.prepare(ps);
	
	return ps.blockSize;
}

