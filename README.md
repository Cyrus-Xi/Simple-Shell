A-Simple-Shell
==============

A simple shell written in C for my Operating Systems course.

The prompt will be the user’s username followed by @ followed by the current working directory: e.g., “cyrus.xi@/home/cyrus.xi/Courses --> “. The usual bash commands (like ls, ps) are supported, as well as the built-ins “cd”, “back”, and “forward” which operate according to the project specifications. Redirection is also supported (even with commands with multiple options), albeit with a different syntax.

Built-ins: “cd”, “back”, and “forward”. They operate according to the project specifications. The number of directories that can be added to the list of visited directories is set at the constant NAVLIMIT. When the list is full, the current elements are “pushed down” the list (such that the earliest directory added is deleted) and the next visited directory is added to the last slot. When “cd” is used with a non-existent directory as an argument, an appropriate error is returned and that argument is not added to the list of visited directories. Similarly, when “back” is used when the current directory is already the earliest visited, then an appropriate error is returned (the converse with “forward”) and the current directory does not change. The commands “back” and “forward” do not modify the list of visited directories; when “cd” is used, it resets the index (to the last visited) used by “back” and “forward” to traverse the list. "cd" with no provided argument changes directory to home directory. Finally, before “cd” is called, the current directory is not the first visited directory; the first visited directory is the first directory entered through actually using “cd”.