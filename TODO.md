# ToDo

The code, as it is, is quite messy. I want to rewrite it a bit in order to
simplify it, to make it more pretty and maintainable and perhaps faster.

Ideas:

- rewrite the token parser. it should be a state machine
- rewrite the passgen parser to use batched operations, meaning that all
  all unicode should be parsed in one go, then all (or 128) tokens should
  be parsed at a time. but still allow it to be used in a streaming manner.
  this should be more optimal for speed and also just lead to cleaner code.
- make the data structures pretty. they should be maintainable, and optionally
  allow for JSON output for debugging
- write benchmarks to make sure that the parser is quick. that's the whole
  reason this is written in C, anyways.
- write a fuzzing backend, using AFL or the like, to ensure there are no bugs
  that can crash the parser.
- optimise the testing system, perhaps use another testing framework that is
  more actively maintained, test more edge cases and use macros to write clean
  tests.
- package it nicely
- implement wordlist functionality (find a space-efficient way to store large
  word lists?)
