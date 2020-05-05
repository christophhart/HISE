/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<2>
  input: "zero2"
  output: "process2frame2"
  error: ""
  filename: "variadic/process2frame"
END_TEST_DATA
*/

int main(ProcessData<2>& data)
{
    auto& frame = data.toFrameData();
    
    while(frame.next())
    {
        frame[0] = 90.0f;
        frame[1] = 125.0f;
    }
    
    return 0;
}

