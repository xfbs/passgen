# Grammar

The grammar for passgen is, at first glance, similar to that of regular expressions, 
however it is much simplified.

### Tokens

Any valid unicode codepoint is a valid input token. 

> `abßü` → a b ß ü

In addition, there are some special types of tokens. There is a unicode escape sequence, which looks like this, where `E4` stands for the unicode character `0xE4` (expressed in hexadecimal).

> `\u{E4}` → ä

In some cases, characters have to be escaped to be recognised as simple tokens and not control characters. This depends on the context.

> `ab\[\]` → a b [ ]

### Syntax

Simple characters are printed verbatim.

> `abc` → *abc*

Groups are delimited by parentheses and have multiple options, delimited by a pipe ('`|`') character.

> `(abc|def)` → *abc* or *def*

The root element is a group, so parentheses aren't necessary.

> `abc|def` → *abc* or *def*

Sets are groups of possible choices for a character, delimited by square brackets. They can also contain ranges of characters delimited by a dash ('`-`').

> `[abc]` → *a* or *b* or *c*  
> `[0-3]` → *0* or *1* or *2* or *3*

Special characters are special escaped characters which are replaced by something.

> `\p[english]{10,12}` → broullion

#### Modifiers

Modifiers can alter the syntax element they apply to. The optional modifier, denoted by a question mark ('`?`') *after* the element, makes it optional (meaning there is a fifty percent chance it doesn't appear).

> `abc?` → *abc* or *ab*  
> `[ab][de]?` → *ad* or *ae* or *bd* or *be* or *a* or *b*  
> `abc(def)?` → *abc* or *abcdef*

The repetition modifier allows repeating a syntax element an arbitrary amount of times. It uses curly braces and takes either one or multiple numbers. Using a single number repeats the syntax element that many times.

> `abc{3}` → *abccc*  
> `[ab]{2}` → *aa* or *ab* or *ba* or *bb*  
> `(word){2}` → *wordword*

Using two numbers specifies a minimum and maximum amount of repetitions (the number is chosen randomly).

> `abc{1,3}` → *abc* or *abcc* or *abccc*  
> `[ab]{1,2}` → *a* or *b* or *aa* or *ab* or *ba* or *bb*  

#### Examples

For example, to generate an email address with the user part consisting only of
lowercase alphabetical characters, numbers and dots, and the domain consisting
only of lowercase alphabetical characters and a TLD, a pattern could look
something like this:

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    viy4@kdptilen.org


### Formal Grammar

The format grammar of the password pattern grammar is denoted here in BNF form.

> `token` ← `unicode` | '`\`' `unicode` | '`\`' '`u`' '`{`' `hex` '`}`'
>
> `pattern` ← `group-inner`
>
> `group-inner` ← `segment` ('`|`' `group-inner`)? | *empty*
>
> `segment` ← `char` `segment` | `set` `segment` | `group` `segment` | *empty*
>
> `modifiers` ← '`?`' `modifiers` | '`{`' `number` '`}`' `modifiers` | '`{`' `number` '`,`' `number` '`}`' `modifiers` | *empty*
>
> `group` ← '`(`' `group-inner` '`)`' `modifiers`
>
> `char` ← `token` `modifier`
>
> `set` ← '`[`' `set-inner` '`]`' `modifiers`


