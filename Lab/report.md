If you don't return anything in the "pattern {action}" in flex, the lexer will simply ignore the matched pattern and continue searching for other patterns in the input. This might result in unexpected behavior in your compiler, as certain tokens or patterns might be skipped or incorrectly recognized.

In general, it is good practice to always return a token value in the "pattern {action}" block, even if it is just a dummy value to indicate that the pattern was matched. This ensures that all patterns are processed correctly and that the lexer produces a complete and accurate token stream for the parser.