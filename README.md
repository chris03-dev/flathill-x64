# flathill-x64

This is an amateur hobby project about a compiler for my very own programming language.

"flathill" is built on C++, and its output is used as assembly input for the flat assembler (by Tomasz Grysztar). It aims to be a loosely typed, relatively close-to-metal language, with just enough basic high-level features to be usable for the majority of coders, while avoiding the complexity of other high level languages.

Its compiler is currently in a very early stage, so it should NOT be considered a real development tool as of yet. Currently, it supports:

- Scopes
	- Functions
	- Objects (soon)
- Variable declaration
	- Global variables (incomplete)
	- Stack variables
- Basic equation parsing
	- Values
		- Numeric constants
		- Global variables
		- Stack variables
	- Operators
		- Add
		- Subtract
		- Multiply
		- Divide
	- Miscellaneous
		- Parenthesis recursion
