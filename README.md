# flathill-x64

All source code until v0.2.1-alpha are licensed under the BSD 2-Clause Patent license.

All source code since v0.2.2-alpha will be under the Universal Permissive License, version 1.0.

---

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
