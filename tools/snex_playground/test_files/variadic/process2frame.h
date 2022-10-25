/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<2>
  input: "zero2.wav"
  output: "process2frame2.wav"
  error: ""
  filename: "variadic/process2frame"
END_TEST_DATA
*/

int main(ProcessData<2>& data)
{
    auto frame = data.toFrameData();
    
    while(frame.next())
    {
        frame[0] = 0.2f;
        frame[1] = 0.8f;
    }
    
    return 0;
}

