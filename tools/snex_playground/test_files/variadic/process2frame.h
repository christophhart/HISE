/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 215
  error: ""
  filename: "variadic/process2frame"
END_TEST_DATA
*/

span<float, 16> c1 = { 4.0f };
span<float, 16> c2 = { 3.0f };

ProcessData<2> d;// = { c1, c2 };

int main(int input)
{
    d.data[0] = c1;
    d.data[1] = c2;

    float z = 0.0f;
    
    auto& frame = d.toFrameData();
    
    while(frame.next())
    {
        frame[0] = 90.0f;
        frame[1] = 125.0f;
    }
    
    return c1[0] + c2[0];
}

