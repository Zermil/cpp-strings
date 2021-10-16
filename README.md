# cpp-strings
Copy of Linux's utility command "strings" in C++ created as a learning project/exercise.
[Actual Linux "strings" command](https://man7.org/linux/man-pages/man1/strings.1.html)

## Quick start
Usage: strings [FILE] [OPTIONS]

Options:\
`-n=[min_string_size]`\
`-ser=[search_for_string]`

### Compile
```console
> build
```

### Run
```console
> strings test.txt -n=6 -ser=123
```