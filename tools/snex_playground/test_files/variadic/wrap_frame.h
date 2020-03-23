/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 42
  error: ""
  filename: "variadic/wrap_frame"
END_TEST_DATA
*/

struct Test
{
    void reset()
    {
    }
    
    void process(ProcessData& d)
    {
        
    }
    
    void processSingle(span<float, 2>& d)
    {
        d[0] = 0.5f;
        d[1] = 41.f;
    }
};

wrapp::frame<Test> w;

span<float, 16> c1 = { 1.0f };
span<float, 16> c2 = { 52.0f };

ProcessData d;

void setup()
{
    d.data[0] = c1;
    d.data[1] = c2;
}

int main(int input)
{
    setup();
    
    w.process(d);
    
    return 42;
}

