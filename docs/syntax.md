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
arrow from last circle.e right 200%
SEGMENT: oval "SEGMENT" fit
arrow 200%
circle same

arrow from SEGMENT.e right then down 1.5*SEGMENT.ht then left
oval "\"|\"" fit
arrow left even with SEGMENT.w then left then up 1.5*SEGMENT.ht then right to SEGMENT.w
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
SEGMENT: oval "SEGMENT" fit
arrow 200%
oval "\")\"" fit
arrow
circle same

arrow from SEGMENT.e right then down 1.5*SEGMENT.ht then left
oval "\"|\"" fit
arrow left even with SEGMENT.w then left then up 1.5*SEGMENT.ht then right to SEGMENT.w
```

## Segment

Segments start with an optional multiplier (changes the likelihood of the segment to be selected at random) and contain a chain of segment items. Every segment item is optionally followed by modifiers.

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow from last circle.e
box "multiplier" italic fit
arrow 200%
ITEM: oval "ITEM" fit
arrow
MODIFIERS: box "modifiers" italic fit
arrow 200%
circle same

arrow from MODIFIERS.e right then down 1.5*ITEM.ht then left even with ITEM.w then left then up 1.5*ITEM.ht then right to ITEM.w
```

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

arrow from LN.e right then down 1.5*CN.ht then left even with CN.w then left then up 1.5*CN.ht then right to CN.w
```

## Range

Ranges are inclusive and consist of a start character optionally followed by dash `-` and a stop character. For example, the range `a` only contains the single character `a`, whereas the range `a-c` contains the characters `a`, `b` and `c`. To create a range consisting of a literal dash, it must be escaped (`\-`).

```kroki-pikchr
linerad = 10px
linewid *= 0.5

circle radius 10%
line 300px color white
arrow 100% from last circle.e
box "character" italic fit
arrow right then down then right
oval "\"-\"" fit
arrow right
box "character" italic fit
arrow right then up then right
circle same
arrow from first box.e to last circle.w
```

### Tokens

Any valid unicode codepoint is a valid input token. 

> `abÃÂÃÂ¼` Ã¢ÂÂ a b ÃÂ ÃÂ¼

In addition, there are some special types of tokens. There is a unicode escape sequence, which looks like this, where `E4` stands for the unicode character `0xE4` (expressed in hexadecimal).

> `\u{E4}` Ã¢ÂÂ ÃÂ¤

In some cases, characters have to be escaped to be recognised as simple tokens and not control characters. This depends on the context.

> `ab\[\]` Ã¢ÂÂ a b [ ]

### Syntax

Simple characters are printed verbatim.

> `abc` Ã¢ÂÂ *abc*

Groups are delimited by parentheses and have multiple options, delimited by a pipe ('`|`') character.

> `(abc|def)` Ã¢ÂÂ *abc* or *def*

The root element is a group, so parentheses aren't necessary.

> `abc|def` Ã¢ÂÂ *abc* or *def*

Sets are groups of possible choices for a character, delimited by square brackets. They can also contain ranges of characters delimited by a dash ('`-`').

> `[abc]` Ã¢ÂÂ *a* or *b* or *c*  
> `[0-3]` Ã¢ÂÂ *0* or *1* or *2* or *3*

Special characters are special escaped characters which are replaced by something.

> `\p[english]{10,12}` Ã¢ÂÂ broullion

#### Modifiers

Modifiers can alter the syntax element they apply to. The optional modifier, denoted by a question mark ('`?`') *after* the element, makes it optional (meaning there is a fifty percent chance it doesn't appear).

> `abc?` Ã¢ÂÂ *abc* or *ab*  
> `[ab][de]?` Ã¢ÂÂ *ad* or *ae* or *bd* or *be* or *a* or *b*  
> `abc(def)?` Ã¢ÂÂ *abc* or *abcdef*

The repetition modifier allows repeating a syntax element an arbitrary amount of times. It uses curly braces and takes either one or multiple numbers. Using a single number repeats the syntax element that many times.

> `abc{3}` Ã¢ÂÂ *abccc*  
> `[ab]{2}` Ã¢ÂÂ *aa* or *ab* or *ba* or *bb*  
> `(word){2}` Ã¢ÂÂ *wordword*

Using two numbers specifies a minimum and maximum amount of repetitions (the number is chosen randomly).

> `abc{1,3}` Ã¢ÂÂ *abc* or *abcc* or *abccc*  
> `[ab]{1,2}` Ã¢ÂÂ *a* or *b* or *aa* or *ab* or *ba* or *bb*  

#### Examples

For example, to generate an email address with the user part consisting only of
lowercase alphabetical characters, numbers and dots, and the domain consisting
only of lowercase alphabetical characters and a TLD, a pattern could look
something like this:

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    viy4@kdptilen.org


### Formal Grammar

The format grammar of the password pattern grammar is denoted here in BNF form.

> `token` Ã¢ÂÂ `unicode` | '`\`' `unicode` | '`\`' '`u`' '`{`' `hex` '`}`'
>
> `pattern` Ã¢ÂÂ `group-inner`
>
> `group-inner` Ã¢ÂÂ `segment` ('`|`' `group-inner`)? | *empty*
>
> `segment` Ã¢ÂÂ `char` `segment` | `set` `segment` | `group` `segment` | *empty*
>
> `modifiers` Ã¢ÂÂ '`?`' `modifiers` | '`{`' `number` '`}`' `modifiers` | '`{`' `number` '`,`' `number` '`}`' `modifiers` | *empty*
>
> `group` Ã¢ÂÂ '`(`' `group-inner` '`)`' `modifiers`
>
> `char` Ã¢ÂÂ `token` `modifier`
>
> `set` Ã¢ÂÂ '`[`' `set-inner` '`]`' `modifiers`


