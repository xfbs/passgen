# Passgen

Passgen generates **random sequences** from a dialect of **regular
expressions**. It is suitable for generating **secure passphrases** that are
**memorable** but have **high-entropy**. It is available for download as a
command-line utility. For a quick overview of how it works, see
[usage](usage.md).

<div class="grid cards" markdown>

-   :material-security:{ .lg .middle } **Secure**

    ---

    Passgen can use any source of randomness, defaulting to the system
    randomness generator.

    [:octicons-arrow-right-24: Security][security]

-   :material-file-edit:{ .lg .middle } **Customizable**

    ---

    Passgen supports a dialect of [regular expressions][regex] to be able to generate
    any kinds of sequences.

    [:octicons-arrow-right-24: Grammar][grammar]

-   :material-speedometer:{ .lg .middle } **Simple**

    ---

    Passgen comes with many useful and secure presets out-of-the box that make
    generating secure passphrases simple.

    [:octicons-arrow-right-24: Presets][presets]

-   :material-scale-balance:{ .lg .middle } **Open Source (MIT)**

    ---

    Passgen is [open-source][repo] and permissively licensed under the [MIT
    license][license].

    [:octicons-arrow-right-24: License][license]

[grammar]: syntax.md
[security]: development/security.md
[presets]: presets.md
[repo]: https://gitlab.com/xfbs/passgen
[license]: https://gitlab.com/xfbs/passgen/-/blob/master/LICENSE.md

</div>

## Getting Started

If you want to try it out, take a look at the [releases](nightly.md) section
where you can find signed tarballs and packages for your platform operating
system. You can also [compile it yourself](development/building.md), if you wish.

If you want to help develop it, see [development](development/index.md), which has
resources and links to get you started.

[regex]: https://en.wikipedia.org/wiki/Regular_expression
