# Entropy

Entropy is the measure of chaos in a system. In computer science, we use the
term entropy when talking about random data as measuring how many bits of
randomness something contains. For example, a lowercase eight-letter password
such as `frjnqaip` contains less entropy than an alphanumeric eight-character
password that has both upper- and lowercase letters, such as `HH8rfTiF`,
despite them having the same length. This is because there are more
possibilities of the latter.

Passgen can calculate the entropy of the passphrases it generates. This can
give you a useful estimate of how much computing power would be required to
crack the passphrase, if an adversary knew the exact pattern you were using to
generate it. You can use the `--entropy` flag to tell passgen to compute
the entropy as it is generating passwords. For example:

```
$ passgen -e '[a-z]{8}'
entropy: 37.603518 bits
frjnqaip
$ passgen -e '[A-Za-z0-9]{8}'
entropy: 47.633570 bits
Q5cO12sF
```

In general, the longer a passphrase is, or the more choice it has (more
possible letters, longer wordlist), the higher the entropy it, and therefore
the more secure it is.

## Calculator

Given some public data, it is possible to estimate how much time an adversary would need to crack a passphrase of a given entropy, assuming that the adversary knows the pattern used to generate it. 

| Name | Value |
| :-- | --- |
| Entropy | <input type="range" min="0" max="256" value="100" class="slider" id="entropy-input"><br /><span id="entropy-value"></span> |
| Algorithm | <select name="algorithm" id="algorithm-input"><option value="sha256" default>SHA256</option><option value="md5">MD5</option><option value="scrypt">SCrypt</option></select> |
| Budget | <input type="range" min="0" max="1000" value="750" class="slider" id="budget-input"><br /><span id="budget-value"></span> |
| Time | <span id="time-value"></span> |

<script>
// get int slider value from a slider with a given ID
function get_slider_value(id) {
    var slider = document.getElementById(id);
    return parseInt(slider.value);
}

// set element's inner HTML.
function set_element_html(id, html) {
    var element = document.getElementById(id);
    element.innerHTML = html;
}

// adjust a linear value [0, 1.0] to a log scale [0, target].
function log_scale(target, value) {
    return Math.round(Math.pow(target, value));
}

// normalize a time into a number and a unit
function time_normalize(time) {
    var unit = "seconds";
    if(time >= 60) {
        time /= 60;
        unit = "minutes";
        if(time >= 60) {
            time /= 60;
            unit = "hours";
            if(time >= 24) {
                time /= 24;
                unit = "days";
                if(time >= 365) {
                    time /= 365;
                    unit = "years";
                    if(time >= 1000) {
                        time /= 1000;
                        unit = "millenium";
                        if(time >= 1000000) {
                            time /= 1000000;
                            unit = "eon";
                        }
                    }
                }
            }
        }
    }

    return [Math.round(time), unit];
}

// round a number to some precision
function round_to(value, precision) {
    var multiplier = Math.pow(10, precision);
    return Math.round(value * multiplier) / multiplier;
}

function recompute_entropy() {
    var entropy = get_slider_value("entropy-input");
    set_element_html("entropy-value", `${entropy} bits`);

    let algorithm = document.getElementById("algorithm-input").value;

    let budget = log_scale(100000000, get_slider_value("budget-input") / 1000);
    set_element_html("budget-value", `$${budget}`);

    // https://gist.github.com/Chick3nman/32e662a5bb63bc4f51b847bb422222fd
    let gpus = budget / 1600;

    var hashrate = 0;
    if(algorithm == "sha256") {
        hashrate = 21960300000 * gpus;
    }
    if(algorithm == "md5") {
        hashrate = 164000000000 * gpus;
    }
    if(algorithm == "scrypt") {
        hashrate = 7126 * gpus;
    }

    let [time, unit] = time_normalize(Math.pow(2, entropy) / hashrate);

    set_element_html("time-value", `${time} ${unit}`);
}

function hook_slider(id) {
    document.getElementById(id).oninput = recompute_entropy;
    document.getElementById(id).onchange = recompute_entropy;
}

var input_sliders = ["entropy-input", "budget-input", "algorithm-input"];
input_sliders.forEach((slider) => hook_slider(slider));

recompute_entropy();
</script>

## Recommendation

We recommend to use passphrases with at least 100 bits of entropy. Every bit doubles the computational effort needed to crack the passphrase. It is better to err on the side of caution. Also keep in mind that often times, the passphrase is not the weakest link in the chain, but the human is.

<figure markdown>
[![Security](https://imgs.xkcd.com/comics/security.png)](https://xkcd.com/538/)
<figcaption markdown>Security ([XKCD 538](https://xkcd.com/538/))</figcaption>
</figure>
