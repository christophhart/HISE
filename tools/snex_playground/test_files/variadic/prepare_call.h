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
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    int x = 0;
    
    void prepare(PrepareSpecs ps)
    {
        x = ps.blockSize;
        ps.blockSize = 100;
    }
};

struct Y
{
    DECLARE_NODE(Y);

    template <int P> void setParameter(double v)
    {
      
    }

    int value = 0;
    
    void prepare(PrepareSpecs ps)
    {
        value = ps.blockSize * 2;
    }
};

container::chain<parameter::empty, wrap::fix<2, X>, Y> c;


int main(int input)
{
	PrepareSpecs ps;
  ps.sampleRate = 44100.0;
  ps.blockSize = 512;
  ps.numChannels = 2;
	
	c.prepare(ps);
	
    return c.get<0>().x;

//	return ps.blockSize;
}

