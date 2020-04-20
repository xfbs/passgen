# Grammar

The grammar for passgen is similar to that of regular expressions, however it
is simplified.

## Characters

Simple characters are printed verbatim.

    $ passgen 'abc'
    abc

## Sets of Characters

Square brackets designate a character set. For example, this will choose any of
the characters a, b or c.

    $ passgen '[abc]'
    b

Character sets also support ranges, this can be used to generate, say, a single
alphanumeric character.

    $ passgen '[a-z0-9]'
    g

## Groups

Groups can have multiple choices, separated by a pipe character.

    $ passgen '(a|the)'
    a

## Repetition

Any item (character, set, group) can be repeated.

    $ passgen 'a{10}'
    aaaaaaaaaa

Repetition can also be expressed as a range, for example, to repeat it a
minimum of three times and a maximum of seven times.

    $ passgen 'x{3,7}'
    xxxxx

## Maybe Operator

Any item can also be marked to be optional. For example, to make the group 'Mr.
' optional, it can be put after the group closing.

    $ passgen '(Mr. )?Donovan'
    Donovan

## Examples

For example, to generate an email address with the user part consisting only of
lowercase alphabetical characters, numbers and dots, and the domain consisting
only of lowercase alphabetical characters and a TLD, a pattern could look
something like this:

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    viy4@kdptilen.org

