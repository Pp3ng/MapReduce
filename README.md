# Multi-threaded Word Count Program

## Introduction

This is a multi-threaded word count program written in C++. The program uses a MapReduce-style approach to process multiple text files in parallel, count the occurrences of all words, and output the results sorted alphabetically.

## Features

- Supports processing multiple text files simultaneously
- Uses multi-threading for parallel processing, improving efficiency
- Case-insensitive counting, converting all words to lowercase
- Uses regular expressions to match words, supporting various text formats
- Thread-safe word counting
- Results sorted alphabetically
- Uses C++20 features such as ranges and barriers for improved performance and readability

## Usage

1. Compile the program:

   ```
   g++ -std=c++20 -pthread mapReduce.cpp -o wordcount
   ```

2. Run the program:
   ```
   ./wordcount <file1> <file2> ... <fileN>
   ```
   Where `<file1>`, `<file2>`, ..., `<fileN>` are the paths to the text files to be processed.

## Example

```
./wordcount sample1.txt sample2.txt sample3.txt
```

## Output

The program will output each word and its total count across all input files, sorted alphabetically:

```
Word counts:
a: 150
an: 75
the: 200
...
```

## Dependencies

- C++20
- Compiler supporting `<filesystem>` `<ranges>` and `<barrier>` headers

## Notes

- The program will ignore non-existent files and continue processing other files
- If exceptions occur during processing, the program will catch and output error messages but continue processing other files