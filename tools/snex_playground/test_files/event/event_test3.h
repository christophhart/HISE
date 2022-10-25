/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<1>
  input: "zero.wav"
  output: "zero.wav"
  error: ""
  filename: "event/event_test3"
  events: [
      { "Type": "NoteOn", "Channel": 4, "Value1": 62, "Value2": 32, "Timestamp": 0 },
      { "Type": "NoteOff", "Channel": 8, "Value1": 61, "Value2": 0, "Timestamp": 512 }
  ]
END_TEST_DATA
*/

int main(ProcessData<1>& data)
{
	int sum = 0;

	for(auto& e: data.toEventData())
		sum += e.getChannel();
	
	return !(sum == 12);
}


