/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 9
  output: 1
  error: ""
  filename: "basic/if_globalwrite"
END_TEST_DATA
*/


float x = 1.0f; 

float main(float input) 
{ 
	x = 1.0f; 
	
	if (input < -0.5f) 
		x = 12.0f; 
		
	return x; 
}