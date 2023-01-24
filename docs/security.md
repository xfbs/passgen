# Security

<figure markdown>
[![Artist's rendition of randomness, dice](images/randomness.jpg){ width="600px" }](https://unsplash.com/photos/kgRBoAKq-4E)
<figcaption>Artist's rendition of randomness.</figcaption>
</figure>

Passgen generates random sequences, and in order to do so, it needs access to
secure, random data. By default, it will use whatever is the best source of
randomness that is availalble on the system. For Linux, this means using the
[getrandom][] system call, and falling back to `/dev/urandom`, which are both
the [most secure source of randomness on the platform][linuxrandom]. On
macOS that means using [arc4random\_buf][arc4random] to get random data, which
is equally secure. Both of these methods will pool randomness from various sources,
such as jitter, interrupt timings and the CPU's built-in randomness generator.

## Other Sources

!!! warning "Using other sources of randomness is not recommended"

    Using alternative sources of randomness is not recommended, unless you know what you are doing.
    These are mainly implemented for testing purposes.
    The default system randomness generator produces the highest quality random data, better than
    any userspace PRNG. Do this only if you know what you are doing, for example if you have a 
    hardware random number generator.

Passgen can also use other source of randomness if instructed.  The
command-line utility takes the `--random RANDOM` flag, which allows for
specifying an alternate source of randomness. Some randomness sources take an
argument.

| Syntax | Description |
| --- | --- |
| `system` | Use the system random number generator (default). |
| `file:/path/to/file` | Read randomness from the specified file. This can be used to read randomness from a different device, such as `/dev/random`, or a hardware randomness generator. |
| `zero` | Does not return any random data, instead simply returns zeroes. Used only for testing. |
| `xorshift:seed` | [XorShift][xorshift]. Takes a non-zero number as seed. Produces very poor quality randomness. |

[xorshift]: https://en.wikipedia.org/wiki/Xorshift
[getrandom]: https://man7.org/linux/man-pages/man2/getrandom.2.html
[arc4random]: https://www.freebsd.org/cgi/man.cgi?query=arc4random_buf&sektion=3&n=1
[linuxrandom]: https://www.2uo.de/myths-about-urandom/
