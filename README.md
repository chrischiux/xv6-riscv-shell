# xv6 RISC V Custom Shell Interface
A custom shell program for the xv6 operating system for RISC V

## Functionality
- Pipe `|`
- Input/Output redirect `>` `<`
- Sequential shell commands `;`
- Combination of all above together

## Example commands
- Change directory 
`cd home`
- Writes character strings to standard output 
`echo hello world`
- Output redirection
`echo "Hello world" > temp`
- Input redirection
`cat < temp`
- Pipes
`cat README | grep github`
`ls | grep test | cat`
- sequential shell commands
`cat README; cat myoutput`