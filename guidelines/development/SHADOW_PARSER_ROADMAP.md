# Roadmap for shadow parser rewrite

## Features

- [ ] Implement predefined objects
- [ ] add RuntimeIdCheck as classification, demote to warning on Standalone code
- [ ] add Warning / Error numbers - eg. "E1002", "W4090" with hise-cli -geterror E1002 explaining what the issue is

## Parser Functionality

- [X] fix includes: File2.js refers to a variable defined in File1.js 
- [X] check control structures, don't execute loops (OK?). 
- [ ] implement anonymous captures including => syntax and other call sites (eg. Array.sort())

## Big Parser Cleanup:

- [X] remove swapping the namespace data structures OK
- [X] remove Expression::hasSideEffects(), getObjectTypeInformat() OK
- [X] remove custom handling in various expression types if possible OK
- [X] remove ApiValidator altogether! OK
- [ ] remove all callscope stuff, move enums to DiagnosticBase
- [X] make RootObject::ObjectTypeMap not owned by the root object but a standalone object! OK
- [X] cleanup all ObjectTypeMap structures, all is handled by ScopedStack now
- [ ] remove all #if 0s
- [ ] make all diagnostic stuff hide behind HISE_SHADOW_PARSER_ENABLED

## Diagnostics:

- [ ] migrate all DiagnosticBase lambdas to ApiDiagnostics.cpp
- [ ] migrate Graphics validations...
- [ ] make ApiClass not a DiagnosticBase method
- [ ] check DiagnosticIdeas for additional input