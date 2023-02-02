# Syntax

The grammar for passgen patterns is inspired by that of regular expressions,
with some simplifications and some modifications that are relevant for
generating random data. This page specifies that syntax of passgen.

## Pattern

Patterns are basically like a group but without the enclosing parenthesis. They consist of segments that are separated by the pipe character ++bar++. A segment is chosen at random. If you need a literal pipe character, you must escape it ++backslash+bar++.

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow 200% from last circle.e
CN: oval "SEGMENT" fit
LN: line 200%
arrow from LN.e right 200%
circle same

arrow from LN.e right then down 1.5*CN.ht then left
oval "|" fit
arrow left even with CN.e
oval "SEGMENT" fit
arrow left then up 1.5*CN.ht then right to CN.w
```

## Group

Groups are enclosed in parentheses and consist of segments that are separated by the pipe character ++bar++. A segment is chosen at random. If you need a literal pipe character, you must escape it ++backslash+bar++.

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow from last circle.e
oval "\"(\"" fit
arrow 200%
CN: oval "SEGMENT" fit
LN: line 200%
arrow from LN.e right 200%
oval "\")\"" fit
arrow
circle same

arrow from LN.e right then down 1.5*CN.ht then left
oval "\"|\"" fit
arrow left even with CN.e
oval "SEGMENT" fit
arrow left then up 1.5*CN.ht then right to CN.w
```

## Group Inner



## Segment


## Segment Item


## Set

Sets consist of one or more ranges and are enclosed in square brackets.

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow from last circle.e
oval "\"[\"" fit
arrow 200%
CN: oval "RANGE" fit
LN: line 0%
arrow from LN.e right 200%
oval "\"]\"" fit
arrow
circle same

arrow from LN.e right then down 1.5*CN.ht then left even with CN.e
oval "RANGE" fit
arrow left then up 1.5*CN.ht then right to CN.w
```

## Range

Ranges are inclusive and consist of a start character optionally followed by dash `-` and a stop character. For example, the range `a` only contains the single character `a`, whereas the range `a-c` contains the characters `a`, `b` and `c`. To create a range consisting of a literal dash, it must be escaped (`\-`).

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow 100% from last circle.e
box "start" italic fit
arrow right then down then right
oval "\"-\"" fit
arrow right
box "start" italic fit
arrow right then up then right
circle same
arrow from first box.e to last circle.w
```

### Tokens

Any valid unicode codepoint is a valid input token. 

> `abÃÃ¼` â a b Ã Ã¼

In addition, there are some special types of tokens. There is a unicode escape sequence, which looks like this, where `E4` stands for the unicode character `0xE4` (expressed in hexadecimal).

> `\u{E4}` â Ã¤

In some cases, characters have to be escaped to be recognised as simple tokens and not control characters. This depends on the context.

> `ab\[\]` â a b [ ]

### Syntax

Simple characters are printed verbatim.

> `abc` â *abc*

Groups are delimited by parentheses and have multiple options, delimited by a pipe ('`|`') character.

> `(abc|def)` â *abc* or *def*

The root element is a group, so parentheses aren't necessary.

> `abc|def` â *abc* or *def*

Sets are groups of possible choices for a character, delimited by square brackets. They can also contain ranges of characters delimited by a dash ('`-`').

> `[abc]` â *a* or *b* or *c*  
> `[0-3]` â *0* or *1* or *2* or *3*

Special characters are special escaped characters which are replaced by something.

> `\p[english]{10,12}` â broullion

#### Modifiers

Modifiers can alter the syntax element they apply to. The optional modifier, denoted by a question mark ('`?`') *after* the element, makes it optional (meaning there is a fifty percent chance it doesn't appear).

> `abc?` â *abc* or *ab*  
> `[ab][de]?` â *ad* or *ae* or *bd* or *be* or *a* or *b*  
> `abc(def)?` â *abc* or *abcdef*

The repetition modifier allows repeating a syntax element an arbitrary amount of times. It uses curly braces and takes either one or multiple numbers. Using a single number repeats the syntax element that many times.

> `abc{3}` â *abccc*  
> `[ab]{2}` â *aa* or *ab* or *ba* or *bb*  
> `(word){2}` â *wordword*

Using two numbers specifies a minimum and maximum amount of repetitions (the number is chosen randomly).

> `abc{1,3}` â *abc* or *abcc* or *abccc*  
> `[ab]{1,2}` â *a* or *b* or *aa* or *ab* or *ba* or *bb*  

#### Examples

For example, to generate an email address with the user part consisting only of
lowercase alphabetical characters, numbers and dots, and the domain consisting
only of lowercase alphabetical characters and a TLD, a pattern could look
something like this:

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    viy4@kdptilen.org


### Formal Grammar

The format grammar of the password pattern grammar is denoted here in BNF form.

> `token` â `unicode` | '`\`' `unicode` | '`\`' '`u`' '`{`' `hex` '`}`'
>
> `pattern` â `group-inner`
>
> `group-inner` â `segment` ('`|`' `group-inner`)? | *empty*
>
> `segment` â `char` `segment` | `set` `segment` | `group` `segment` | *empty*
>
> `modifiers` â '`?`' `modifiers` | '`{`' `number` '`}`' `modifiers` | '`{`' `number` '`,`' `number` '`}`' `modifiers` | *empty*
>
> `group` â '`(`' `group-inner` '`)`' `modifiers`
>
> `char` â `token` `modifier`
>
> `set` â '`[`' `set-inner` '`]`' `modifiers`


