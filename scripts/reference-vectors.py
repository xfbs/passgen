#!/usr/bin/env python3
"""Reference implementation of Passgen's deterministic modes.

Generates the test vectors in `src/tests/vectors.inc`, which pin down what the
deterministic randomness sources (master passphrase mode in particular) have
to output. That output must never change, as people rely on it to get their
passphrases back.

This does not share any code with Passgen: the key derivation uses the
reference implementation of Argon2 (argon2-cffi), BLAKE2b is the one of
Python, the stream cipher is the one from OpenSSL (cryptography), and parsing
and generating from patterns is reimplemented here. If the tests pass, two independent implementations agree.

Usage:

    pip install argon2-cffi cryptography
    ./scripts/reference-vectors.py > src/tests/vectors.inc
    ./scripts/reference-vectors.py --cli    # for scripts/test-passgen-cli.sh

Only add vectors. If an existing one changes, that is a bug (in Passgen or in
here), unless the change is intended and announced as a breaking change.
"""

import hashlib
import sys

from argon2.low_level import Type, core, ffi
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms

# Version 1 of master passphrase mode (see include/passgen/master.h): the
# salt, and the memory (in KiB) and passes of Argon2id for every level.
MASTER_SALT = b"passgen/master/1"
MASTER_LEVELS = {1: (64 * 1024, 3), 2: (256 * 1024, 3), 3: (1024 * 1024, 3)}

CHACHA20_DEFAULT_IV = b"passgen\0"


def argon2id(password, salt, secret, memory, passes):
    """Derives a 32 byte key, with one lane and no associated data."""
    out = ffi.new("uint8_t[]", 32)
    # these have to be kept alive until the call is done
    c_password = ffi.new("uint8_t[]", password)
    c_salt = ffi.new("uint8_t[]", salt)
    c_secret = ffi.new("uint8_t[]", secret) if secret else ffi.NULL
    context = ffi.new(
        "argon2_context *",
        dict(
            out=out,
            outlen=32,
            pwd=c_password,
            pwdlen=len(password),
            salt=c_salt,
            saltlen=len(salt),
            secret=c_secret,
            secretlen=len(secret),
            ad=ffi.NULL,
            adlen=0,
            t_cost=passes,
            m_cost=memory,
            lanes=1,
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


def master_key(version, level, passphrase, seed):
    """The master key, from the master passphrase and the seed (bytes)."""
    assert version == 1
    memory, passes = MASTER_LEVELS[level]
    return argon2id(passphrase, MASTER_SALT, seed, memory, passes)


def master_hash(key):
    """What tells whether a master passphrase is the right one."""
    return hashlib.blake2b(b"passgen/hash", key=key, digest_size=32).digest()


def master_site_key(key, domain, user):
    """The key of the stream cipher, for a domain and a user (bytes)."""
    message = b"passgen/site"
    for part in (domain, user):
        message += len(part).to_bytes(8, "little") + part
    return hashlib.blake2b(message, key=key, digest_size=32).digest()


def master_fields(rest):
    """Splits what follows `master:` at colons, where `\\:` is a colon and
    `\\\\` a backslash."""
    fields = [""]
    chars = iter(rest)
    for c in chars:
        if c == "\\":
            c = next(chars, None)
            if c not in (":", "\\"):
                raise ValueError(rest)
            fields[-1] += c
        elif c == ":":
            fields.append("")
        else:
            fields[-1] += c
    return fields


def master_escape(field):
    return field.replace("\\", "\\\\").replace(":", "\\:")


class ChaCha20:
    """Keystream of the original ChaCha20 (64 bit counter, 64 bit nonce)."""

    def __init__(self, key, iv):
        assert len(key) == 32 and len(iv) == 8
        self.key = key
        self.iv = iv
        # OpenSSL takes the counter and the nonce as one 16 byte value
        cipher = Cipher(algorithms.ChaCha20(key, bytes(8) + iv), mode=None)
        self.encryptor = cipher.encryptor()

    def read(self, size):
        return self.encryptor.update(bytes(size))

    def stream(self, index):
        """The keystream for the IV, as a 64 bit little-endian number, XOR
        the index."""
        iv = int.from_bytes(self.iv, "little") ^ index
        return ChaCha20(self.key, iv.to_bytes(8, "little"))


class Xorshift:
    """The xorshift64 generator, output as little-endian 64 bit values."""

    def __init__(self, seed):
        self.seed = seed
        self.state = seed

    def stream(self, index):
        """The generator with the seed XOR the index after the finalizer of
        splitmix64, or 2^64 - 1 instead of zero."""
        mask = (1 << 64) - 1
        mix = (index * 0x9E3779B97F4A7C15) & mask
        mix = ((mix ^ (mix >> 30)) * 0xBF58476D1CE4E5B9) & mask
        mix = ((mix ^ (mix >> 27)) * 0x94D049BB133111EB) & mask
        mix ^= mix >> 31
        return Xorshift((self.seed ^ mix) or mask)

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


def strict_hex(digits, size, description):
    """Exactly `2 * size` hex digits, in either case, as bytes."""
    if len(digits) != 2 * size or any(
        c not in "0123456789abcdefABCDEF" for c in digits
    ):
        raise ValueError(description)
    return bytes.fromhex(digits)


def open_source(description):
    """Opens a randomness source from a description, in the syntax of the
    `--random` option."""
    kind, _, rest = description.partition(":")
    if kind == "xorshift":
        return Xorshift(int(rest))
    if kind == "chacha20":
        # the key is exactly 64 hex digits, and the IV, if there is one,
        # exactly 16. without one, the default IV is used.
        key, colon, iv = rest.partition(":")
        key = strict_hex(key, 32, description)
        iv = strict_hex(iv, 8, description) if colon else CHACHA20_DEFAULT_IV
        return ChaCha20(key, iv)
    if kind == "master":
        # version, level, passphrase, seed (in hex), domain and user, all of
        # which are there.
        fields = master_fields(rest)
        if len(fields) != 6:
            raise ValueError(description)
        version, level, passphrase, seed, domain, user = fields
        if version != "1" or level not in ("1", "2", "3"):
            raise ValueError(description)
        passphrase, domain, user = (x.encode() for x in (passphrase, domain, user))
        if any(len(part) > 128 for part in (passphrase, domain, user)):
            raise ValueError(description)
        seed = bytes.fromhex(seed) if seed else b""
        if len(seed) > 64:
            raise ValueError(description)
        key = master_key(1, int(level), passphrase, seed)
        return ChaCha20(master_site_key(key, domain, user), CHACHA20_DEFAULT_IV)
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
    # one of the segments is chosen. if there is only one, that takes no
    # randomness, as there is nothing to choose.
    if len(segments) == 1:
        segment = segments[0]
    else:
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
            # a single word takes no randomness, as there is nothing to
            # choose from.
            if len(words) == 1:
                output += words[0]
            else:
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
            # a set of a single character (also when it is listed more
            # than once) takes no randomness.
            if len(set(characters)) == 1:
                output += chr(characters[0])
            else:
                output += chr(characters[random.below(len(characters))])
    return output


def generate(description, pattern, wordlists, stream=0):
    segments, _ = parse_group(pattern, 0, True)
    source = open_source(description).stream(stream)
    return generate_group(Random(source), segments, wordlists)


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

def master(passphrase, domain="", user="", level=1, seed=""):
    fields = [str(level), passphrase, seed, domain, user]
    for i in (1, 3, 4):
        fields[i] = master_escape(fields[i])
    return "master:1:" + ":".join(fields)


SOURCES = [
    # master passphrase only, with a domain, and with a domain and a user
    master("master"),
    master("master", "example.com"),
    master("master", "example.com", "user"),
    # every input matters
    master("other", "example.com", "user"),
    master("master", "example.org", "user"),
    master("master", "example.com", "admin"),
    master("master", "example.com", "user", level=2),
    master("master", "example.com", "user", seed="00"),
    master("master", "example.com", "user", seed="73656564"),
    # a user without a domain is not the same as a domain without a user
    master("master", "", "user"),
    master("master", "user", ""),
    # where one field ends and the next starts matters
    master("master", "ab", "c"),
    master("master", "a", "bc"),
    # colons and backslashes, which are escaped
    master("pass:word\\", "example.com:8443", "us\\er"),
    # not only ASCII
    master("pässwörd", "exämple.com", "üser"),
    master("correct horse battery staple", "example.com"),
    # the other deterministic sources
    # the key and IV are in hex. this key is the bytes of `key`, padded with
    # zeroes, and the IVs are `iv` padded with zeroes, and all zeroes.
    "chacha20:6b65790000000000000000000000000000000000000000000000000000000000",
    "chacha20:6b65790000000000000000000000000000000000000000000000000000000000:6976000000000000",
    "chacha20:6b65790000000000000000000000000000000000000000000000000000000000:0000000000000000",
    # 32 times `a`, and the IV `12345678`
    "chacha20:6161616161616161616161616161616161616161616161616161616161616161:3132333435363738",
    # every byte value of a key, in both cases of hex digits
    "chacha20:000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
    "chacha20:FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF:FFFFFFFFFFFFFFFF",
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
    # empty groups generate nothing, but are chosen from like any other, and
    # what follows them applies to them
    "[a-z]{4}(){3}[a-z]{4}(|)?[a-z]{4}(){0,2}",
    # groups with a single segment and sets of a single character have
    # nothing to choose from, and take no randomness
    "(abc)[a][bb]([0-9]{2}){2}(x|y)[0-9]",
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
        source = master("master", "example.com", "user")
        yield source, pattern, generate(source, pattern, wordlists)


# sources with streams, the indices of streams, and patterns, which generate
# what would be generated for those indices with `-a` or `--offset`. stream
# zero is the same as the vectors above.
STREAMS = [
    ("xorshift:1", [0, 1, 2, 3, 1000, 2**64 - 1]),
    ("xorshift:18446744073709551615", [0, 1, 2]),
    (
        "chacha20:000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        [0, 1, 2, 3, 1000, 2**64 - 1],
    ),
    (
        "chacha20:000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f:FFFFFFFFFFFFFFFF",
        [0, 1, 2],
    ),
    (master("master", "example.com", "user"), [0, 1, 2, 3]),
]
STREAM_PATTERNS = ["[a-z]{16}", "[a-z]{8}(-[a-z0-9]{0,8})?", "\\w{words}{1,3}"]


# master passphrases, levels and seeds, and their hashes.
HASHES = [
    ("master", 1, ""),
    ("master", 2, ""),
    ("other", 1, ""),
    ("master", 1, "73656564"),
    ("pässwörd", 1, ""),
]


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
    print("// HASH(master passphrase, level, seed in hex, expected hash in hex)")
    print("// STREAM(randomness source, index of the stream, pattern, expected output)")
    print()
    for word in WORDS:
        print(f"WORD({c_string(word)})")
    print()
    for passphrase, level, seed in HASHES:
        key = master_key(1, level, passphrase.encode(), bytes.fromhex(seed))
        print(
            f"HASH({c_string(passphrase)}, {level}, {c_string(seed)}, "
            f"{c_string(master_hash(key).hex())})"
        )
    print()
    for source, indices in STREAMS:
        for index in indices:
            for pattern in STREAM_PATTERNS:
                expected = generate(source, pattern, {"words": WORDS}, index)
                print(f"STREAM({c_string(source)}, UINT64_C({index}),")
                print(f"       {c_string(pattern)},")
                print(f"       {c_string(expected)})")
    print()
    for source, pattern, expected in vectors():
        print(f"VECTOR({c_string(source)},")
        print(f"       {c_string(pattern)},")
        print(f"       {c_string(expected)})")


if __name__ == "__main__":
    main()
