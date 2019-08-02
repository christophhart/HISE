# Scriptnode JIT Language

The Scriptnode JIT Language (**snjitlang**) is a extremely simplified subset of the C language family and is used throughout scriptnode for customization behaviour.

Unlike the HiseScript language, which is interpreted, the scriptnode expressions are JIT compiled and run in almost native speed. When the scriptnode graph is exported as Cpp code, the expressions will be then compiled by the standard compiler (which is why it needs to be a strict subset of C / C++).

There are two places where it can be used: in the `core.jit` node, which allows creating fully customizable nodes and in the connection via the `Expression` property which can be used to convert the value send to the connection target.

## Getting started

The easiest way to get to know the language is to use the JIT Playground which offers you a JIT compiler, a code editor with a predefined snippet and shows the assembly output that is fed directly to the CPU.

## Language Reference

The **snjitlang** syntax uses brackets for blocks and semicolons for statements. Comment lines start with `//`, multiline comments with `/** Comment */)`

```cpp

/** This is a 
    multiline comment
*/
{
	x; // a statement
}
```

Function definitions have this syntax:

```cpp
ReturnType functionName(Arguments...)
{
   // body
}
```

Variable names must be a valid identifier, and definitions must initialise the value:

```cpp
type variableName = initialValue;
```

You can define variables in any scope (function scope or global scope).

## Types

Unlike HiseScript, **snjitlang** is strictly typed. However there is a very limited set of available types:

- `int` - Integer numbers
- `float` - single precision floating point numbers, marked with a trailing `f`: `2.012f`
- `double`- double precision floating point numbers
- `bool` - boolean values (just as intermediate type for expression results)
- `block` - float array
- `event` - the HiseEvent

Conversion between the types is done via a C-style cast:

```cpp
// converts a float to an int
int x = (int)2.0f;
```

however, the compiler will generate implicit casts (and warn you about this).

## Operators

### Binary Operations

The usual binary operators are available: 

```cpp
a + b; // Add
a - b; // Subtract
a * b; // Multiply
a / b; // Divide
a % b; // Modulo

a++; // post-increment (no pre increment support!)

!a; // Logical inversion
a == b; // equality
a != b; // inequality
a > b; // greater than
a >= b; // greater or equal
// ...

```

The rules of operator precedence are similar to every other programming language on the planet. You can use parenthesis to change the order of execution:

```cpp
(a + b) * c; // = a * c + b * c
```

### Assignment

Assigning a value to a variable is done via the `=` operator. Other assignment types are supported:

```cpp
int x = 12;
x += 3; // x == 15
x /= 3 // x == 5
```

You can access elements of a `block` via the `[]` operator:

```cpp
block b;
b[12] = 12.0f;
```

There is an out-of-bounds check that prevents read access violations if you use an invalid array index.

### Ternary Operator

There is no `if` statement branching in **snjitlang**, however conditional execution can be achieved using the ternary operator:

```cpp
a ? b : c
```

It supports short-circuiting (so the `false` branch is not executed).

## Function calls

You can call other functions using this syntax: `functionCall(parameter1, parameter2);` It currently supports up to three parameters. Be aware that you can't call functions before defining them:

```cpp
void f1()
{
    f2(); // won't work
}

void f1()
{
    doSomething();
}

void f3()
{
    f1(); // now you can call it
}
```

## Return statement

Functions that have a return type need a return statement at the end of their function body:

```cpp
void f1()
{
    // Do something
    return; // this is optional
}

int f2()
{
	return 42; // must return a int
}

```


## API classes

There are a few inbuilt API classes that offer additional helper functions.

- the `Math` class which contains a set of mathematical functions
- the `Console` class for printing a value to the console
- the `Message` class which contains methods to operate on a HiseEvent.

The syntax for calling the API functions is the same as in HiseScript: `Api.function()`.

```cpp
float x = Math.sin(2.0f);
```

> The `Math` class contains overloaded functions for `double` and `float`, so be aware of implicit casts here.



