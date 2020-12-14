/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "00 Examples/03_functions"
END_TEST_DATA
*/

/** This test shows how to define functions
    and call them in SNEX plus a first introduction
    how to read the assembly output on the right. 
*/

//  A function that just returns 12 and ignores the input
int getValue(int input)
{
	return 12;
}

int main(int input)
{
	// Call the method and forward the input argument.
	return getValue(input);
	
	// Take a look at the assembly on the right: 
	
	/* ; function int main(int main::input)
		 mov eax, 12
		 ret
	*/
	
	// Even without a deep knowledge of assembly you will
	// notice that it somehow "looked" into the other
	// function and inlined the function's body
	// (mov eax, 12 means "put 12 into the return register")
	// If you deactivate `Inlining` on the right, you will
	// see that the assembly output of this function changes to
	
	/* ; function int main(int main::input)
		mov eax, ecx                               
		call 2653812490240                          ; int getValue(int getValue::input)
		ret
	*/
	
	// the magic word being `call` indicating that there is an
	// actual function call happening. Inlining functions is one
	// of the most important compiler optimizations so you should
	// make sure that the hot path of your DSP algorithm contains
	// as less function calls as possible.
}

