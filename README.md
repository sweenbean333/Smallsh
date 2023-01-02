## Overview
In this assignment you will write your own shell in C. The shell will run command line instructions and return the results similar to other shells you have used, but without many of their fancier features.

In this project I have created my own very basic shell similar to the bash shell, that prompts users and runs commands.

The shell supports three built in commands: `exit`, `cd`, and `status`. It will also support comments, which are lines beginning with the # character.

The shell allows for the redirection of standard input and standard output and it supports both foreground and background processes (controllable by the command line and by receiving signals such as "CTRL+Z").

In order to run the program you must first compile the c code. You must use the gcc compiler 
using the c99 standard.

The command to compile a c file is:
gcc -std=c99 [file_name] o- [executable_name]

My file name is smallsh.c and the executable name that I want to use is smallsh.
Therefore, the command will look like:

gcc -std=c99 smallsh.c o- smallsh


### Example

Here is an example of the shell program:

```
$ smallsh
: ls
junk   smallsh    smallsh.c
: ls > junk
: status
exit value 0
: cat junk
junk
smallsh
smallsh.c
: wc < junk > junk2
: wc < junk
       3       3      23
: test -f badfile
: status
exit value 1
: wc < badfile
cannot open badfile for input
: status
exit value 1
: badfile
badfile: no such file or directory
: sleep 5
^Cterminated by signal 2
: status &
terminated by signal 2
: sleep 15 &
background pid is 4923
: ps
PID TTY          TIME CMD
4923 pts/0    00:00:00 sleep
4564 pts/0    00:00:03 bash
4867 pts/0    00:01:32 smallsh
4927 pts/0    00:00:00 ps
:
: # that was a blank command line, this is a comment line
:
background pid 4923 is done: exit value 0
: # the background sleep finally finished
: sleep 30 &
background pid is 4941
: kill -15 4941
background pid 4941 is done: terminated by signal 15
: pwd
/nfs/stak/faculty/b/brewsteb/CS344/prog3
: cd
: pwd
/nfs/stak/faculty/b/brewsteb
: cd CS344
: pwd
/nfs/stak/faculty/b/brewsteb/CS344
: echo 4867
4867
: echo $$
4867
: ^C
: ^Z
Entering foreground-only mode (& is now ignored)
: date
Mon Jan  2 11:24:33 PST 2017
: sleep 5 &
: date
Mon Jan  2 11:24:38 PST 2017
: ^Z
Exiting foreground-only mode
: date
Mon Jan  2 11:24:39 PST 2017
: sleep 5 &
background pid is 4963
: date
Mon Jan 2 11:24:39 PST 2017
: exit $
```

