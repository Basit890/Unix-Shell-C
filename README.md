# My Shell, A Custom UNIX Shell in C

**MyShell** is a simplified UNIX shell implemented in the C programming language as part of an Operating Systems course project. It supports a variety of basic shell functionalities such as command execution, piping, input/output redirection, sequential and conditional execution, and command history.

## Features

- **Basic Command Execution** – Run standard Linux/Unix commands
- **Piping (`|`)** – Chain multiple commands together
- **Input/Output Redirection (`<`, `>`)** – Redirect stdin and stdout
- **Sequential Execution (`;`)** – Run multiple commands in sequence
- **Conditional Execution (`&&`)** – Run the next command only if the previous succeeds
- **Command History** – Stores and recalls previously entered commands
- **Custom Parsing Logic** – Handles whitespace, multiple operators, etc.


## How to Compile

Make sure you're using a Linux environment with `gcc` installed. Then:

```bash
gcc myshell.c -o myshell

Author
Name: Basit Ibrahim
University: BRAC University
Course: CSE 321, Operating Systems
