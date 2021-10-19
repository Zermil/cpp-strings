# cpp-strings
Copy of Linux's utility command "strings" in C++ created as a learning project/exercise.
[Actual Linux "strings" command](https://man7.org/linux/man-pages/man1/strings.1.html)

## Quick start
Usage: ./strings [FILE] [OPTIONS]

Options:\
`-n=[min_string_size]` -> spicifies minimum size of output string\
`-s=[search_string]` -> output only the strings that contain [search_string]\
`-o` -> don't output to console, but to text file instead (text file's name is [FILE]_out.txt)\
`-d` -> display line number for particular string (where it was found)

### Compile
```console
> build
```

### Run
```console
> strings test.txt -n=6 -ser=123
```