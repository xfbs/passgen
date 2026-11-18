#!/usr/bin/env python3
"""Reference implementation of Passgen's deterministic modes.

Generates the test vectors in `src/tests/vectors.inc`, which pin down what the
deterministic randomness sources (master passphrase mode in particular) have
to output. That output must never change, as people rely on it to get their
passphrases back.

This does not share any code with Passgen: the key derivation uses the
reference implementation of Argon2 (argon2-cffi), the stream cipher is the
one from OpenSSL (cryptography), and parsing and generating from patterns is
reimplemented here. If the tests pass, two independent implementations agree.

Usage:

    pip install argon2-cffi cryptography
    ./scripts/reference-vectors.py > src/tests/vectors.inc
    ./scripts/reference-vectors.py --cli    # for scripts/test-passgen-cli.sh

Only add vectors. If an existing one changes, that is a bug (in Passgen or in
here), unless the change is intended and announced as a breaking change.
"""

import sys

from argon2.low_level import Type, core, ffi
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms

# Version 1 of the key derivation (PASSGEN_ARGON2_CONFIG_V1).
ARGON2_SALT = b"passgen/argon2id"
ARGON2_MEMORY_KIB = 19 * 1024
ARGON2_PASSES = 2
ARGON2_LANES = 1

CHACHA20_DEFAULT_IV = b"passgen\0"


def argon2id(password, secret, associated_data):
    """Derives a 32 byte key. Passgen passes the domain as the secret, and
    the token as the associated data."""
    out = ffi.new("uint8_t[]", 32)
    # these have to be kept alive until the call is done
    c_password = ffi.new("uint8_t[]", password)
    c_salt = ffi.new("uint8_t[]", ARGON2_SALT)
    c_secret = ffi.new("uint8_t[]", secret) if secret else ffi.NULL
    c_ad = ffi.new("uint8_t[]", associated_data) if associated_data else ffi.NULL
    context = ffi.new(
        "argon2_context *",
        dict(
            out=out,
            outlen=32,
            pwd=c_password,
            pwdlen=len(password),
            salt=c_salt,
            saltlen=len(ARGON2_SALT),
            secret=c_secret,
            secretlen=len(secret),
            ad=c_ad,
            adlen=len(associated_data),
            t_cost=ARGON2_PASSES,
            m_cost=ARGON2_MEMORY_KIB,
            lanes=ARGON2_LANES,
            threads=1,
            version=0x13,
            allocate_cbk=ffi.NULL,
            free_cbk=ffi.NULL,
            flags=0,
        ),
    )
    result = core(context, Type.ID.value)
    if result != 0:
        raise RuntimeError(f"argon2 failed with {result}")
    return bytes(ffi.buffer(out, 32))


class ChaCha20:
    """Keystream of the original ChaCha20 (64 bit counter, 64 bit nonce)."""

    def __init__(self, key, iv):
        assert len(key) == 32 and len(iv) == 8
        # OpenSSL takes the counter and the nonce as one 16 byte value
        cipher = Cipher(algorithms.ChaCha20(key, bytes(8) + iv), mode=None)
        self.encryptor = cipher.encryptor()

    def read(self, size):
        return self.encryptor.update(bytes(size))


class Xorshift:
    """The xorshift64 generator, output as little-endian 64 bit values."""

    def __init__(self, seed):
        self.state = seed

    def read(self, size):
        assert size % 8 == 0
        out = b""
        mask = (1 << 64) - 1
        for _ in range(size // 8):
            x = self.state
            x ^= (x << 13) & mask
            x ^= x >> 7
            x ^= (x << 17) & mask
            self.state = x
            out += x.to_bytes(8, "little")
        return out


def open_source(description):
    """Opens a randomness source from a description, in the syntax of the
    `--random` option."""
    kind, _, rest = description.partition(":")
    if kind == "xorshift":
        return Xorshift(int(rest))
    if kind == "chacha20":
        key, colon, iv = rest.partition(":")
        key = key.encode()
        if len(key) > 32:
            raise ValueError(description)
        key = key.ljust(32, b"\0")
        # an IV which is given replaces the default one, even an empty one,
        # and is cut off after eight bytes.
        iv = iv.encode()[:8].ljust(8, b"\0") if colon else CHACHA20_DEFAULT_IV
        return ChaCha20(key, iv)
    if kind == "chacha20-argon2":
        # one colon before the master passphrase is skipped, so that
        # `chacha20-argon2::x` has the master passphrase `x`.
        if rest.startswith(":"):
            rest = rest[1:]
        # anything after the token is ignored.
        parts = [part.encode() for part in rest.split(":")] + [b"", b""]
        if any(len(part) > 128 for part in parts[:3]):
            raise ValueError(description)
        key = argon2id(parts[0], parts[1], parts[2])
        return ChaCha20(key, CHACHA20_DEFAULT_IV)
    raise ValueError(description)


class Random:
    """How Passgen turns the bytes of a source into random numbers."""

    def __init__(self, source):
        self.source = source
        self.buffer = b""

    def read(self, size):
        while len(self.buffer) < size:
            self.buffer += self.source.read(1024)
        data, self.buffer = self.buffer[:size], self.buffer[size:]
        return data

    def boolean(self):
        return (self.read(1)[0] & 128) == 0

    def below(self, maximum):
        """A number in 0..maximum-1, using rejection sampling on 64 bit
        values."""
        mask = (1 << (maximum - 1).bit_length()) - 1
        while True:
            value = int.from_bytes(self.read(8), "little") & mask
            if value < maximum:
                return value


# Patterns are parsed into a tree. A group is a list of segments (the
# alternatives), a segment is a list of items, and an item is a tuple of
# (kind, data, minimum, maximum, optional).


def parse_group(pattern, pos, toplevel):
    segments = [[]]
    while pos < len(pattern):
        char = pattern[pos]
        if char == ")":
            assert not toplevel
            return segments, pos + 1
        if char == "|":
            segments.append([])
            pos += 1
        elif char == "(":
            group, pos = parse_group(pattern, pos + 1, False)
            segments[-1].append(["group", group, 1, 1, False])
        elif char == "[":
            ranges, pos = parse_set(pattern, pos + 1)
            segments[-1].append(["set", ranges, 1, 1, False])
        elif char == "{":
            end = pattern.index("}", pos)
            minimum, _, maximum = pattern[pos + 1 : end].partition(",")
            item = segments[-1][-1]
            item[2] = int(minimum)
            item[3] = int(maximum) if maximum else int(minimum)
            pos = end + 1
        elif char == "?":
            segments[-1][-1][4] = True
            pos += 1
        elif char == "\\":
            escaped = pattern[pos + 1]
            if escaped == "w":
                end = pattern.index("}", pos)
                name = pattern[pos + 3 : end]
                segments[-1].append(["wordlist", name, 1, 1, False])
                pos = end + 1
            else:
                segments[-1].append(["literal", escaped, 1, 1, False])
                pos += 2
        else:
            segments[-1].append(["literal", char, 1, 1, False])
            pos += 1
    assert toplevel
    return segments, pos


def parse_set_char(pattern, pos):
    """A character in a set, which may be escaped."""
    if pattern[pos] == "\\":
        pos += 1
    return pattern[pos], pos + 1


def parse_set(pattern, pos):
    ranges = []
    while pattern[pos] != "]":
        start, pos = parse_set_char(pattern, pos)
        if pattern[pos] == "-":
            end, pos = parse_set_char(pattern, pos + 1)
            ranges.append((ord(start), ord(end)))
        else:
            ranges.append((ord(start), ord(start)))
    return ranges, pos + 1


def generate_group(random, segments, wordlists):
    # one of the segments is chosen, even if there is only one
    segment = segments[random.below(len(segments))]
    return "".join(generate_item(random, item, wordlists) for item in segment)


def generate_item(random, item, wordlists):
    kind, data, minimum, maximum, optional = item
    if optional and not random.boolean():
        return ""
    count = minimum
    if minimum != maximum:
        count += random.below(maximum - minimum + 1)
    output = ""
    for _ in range(count):
        if kind == "literal":
            output += data
        elif kind == "group":
            output += generate_group(random, data, wordlists)
        elif kind == "wordlist":
            words = wordlists[data]
            output += words[random.below(len(words))]
        elif kind == "set":
            # every character, as many times as it is listed, in the order
            # of the codepoints. this is what the sorted and merged ranges of
            # a set in Passgen stand for, so `[ba]` is the same as `[ab]`,
            # and in `[aab]`, `a` is twice as likely as `b`.
            characters = sorted(
                codepoint
                for start, end in data
                for codepoint in range(start, end + 1)
            )
            output += chr(characters[random.below(len(characters))])
    return output


def generate(description, pattern, wordlists):
    segments, _ = parse_group(pattern, 0, True)
    return generate_group(Random(open_source(description)), segments, wordlists)


# The wordlist which the tests define under the name `words`.
WORDS = ["alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "größe"]

PROQUINT = "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]"

# The presets which the CLI defines.
PRESETS = {
    "apple1": "([a-zA-Z0-9]{3}-){3}[a-zA-Z0-9]{3}",
    "apple2": "([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}",
    "uuid": "[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}",
    "edge": "[A-Za-f0-9:_\\-]{15}",
    "firefox": "[a-zA-Z0-9]{15}",
    **{
        f"proquint{16 * (n + 1)}": f"{PROQUINT}(-{PROQUINT}){{{n}}}"
        for n in range(8)
    },
}

MASTER = "chacha20-argon2:"

SOURCES = [
    # master passphrase only, with a domain, and with a domain and a token
    MASTER + "master",
    MASTER + "master:example.com",
    MASTER + "master:example.com:user",
    # every input matters
    MASTER + "other:example.com:user",
    MASTER + "master:example.org:user",
    MASTER + "master:example.com:admin",
    # token without a domain
    MASTER + "master::user",
    # one colon before the master passphrase is skipped, so this is the same
    # as "master:example.com"
    MASTER + ":master:example.com",
    # not only ASCII
    MASTER + "pässwörd:exämple.com:üser",
    MASTER + "correct horse battery staple:example.com",
    # the other deterministic sources
    "chacha20:key",
    "chacha20:key:iv",
    # an empty IV is all zeroes, rather than the default one
    "chacha20:key:",
    "chacha20:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:12345678",
    "xorshift:1",
    "xorshift:1234",
    "xorshift:18446744073709551615",
]

PATTERNS = [
    "[a-z]{16}",
    "[a-zA-Z0-9]{32}",
    # choices that are a power of two
    "[0-9a-f]{32}",
    "[ab]{16}",
    # alternatives, repetitions and optional items
    "(abc|def|ghi){4}",
    "(a|b|c|d){8}",
    "[a-z]{4,12}",
    "(ab?c?){6}",
    "(why|how|what)\\?{1,3}",
    # escaped characters in sets, also as the end of a range
    "[a-c\\]\\[\\-]{8}[+-\\-]{8}[Z-\\]]{8}",
    "([a-z]{2,4}-){3}[0-9]{2}",
    # sets are sorted, and a character listed more than once (also through
    # ranges which overlap) is that many times as likely
    "[ba]{16}",
    "[aab]{24}",
    "[a-zc0-9a-f]{24}",
    # characters whose codepoints end in the same byte as characters which
    # have a meaning in patterns
    "(zażółć|gęślą|jaźń){3}[żśŻ]{4}",
    # wordlists
    "\\w{words}(-\\w{words}){3}",
    "\\w{words}{1,3}[0-9]",
    # needs more than one buffer (1024 bytes) of randomness
    "[a-z]{200}",
    "([a-zA-Z0-9]{8}-){40}",
]


def vectors():
    wordlists = {"words": WORDS}
    for source in SOURCES:
        for pattern in PATTERNS:
            yield source, pattern, generate(source, pattern, wordlists)
    # every preset, in master passphrase mode
    for name, pattern in PRESETS.items():
        source = MASTER + "master:example.com:user"
        yield source, pattern, generate(source, pattern, wordlists)


def c_string(value):
    out = ""
    for byte in value.encode():
        if byte in (0x22, 0x5C):
            out += "\\" + chr(byte)
        elif 0x20 <= byte < 0x7F:
            out += chr(byte)
        else:
            out += f"\\{byte:03o}"
    return f'"{out}"'


def main():
    if "--cli" in sys.argv:
        # presets and their expected output with `--random xorshift:1234`,
        # for scripts/test-passgen-cli.sh
        for name, pattern in PRESETS.items():
            print(name, generate("xorshift:1234", pattern, {}))
        return

    print("// Generated by scripts/reference-vectors.py, do not edit.")
    print("//")
    print("// VECTOR(randomness source, pattern, expected output)")
    print("// WORD(word in the wordlist named `words`)")
    print()
    for word in WORDS:
        print(f"WORD({c_string(word)})")
    print()
    for source, pattern, expected in vectors():
        print(f"VECTOR({c_string(source)},")
        print(f"       {c_string(pattern)},")
        print(f"       {c_string(expected)})")


if __name__ == "__main__":
    main()
