# Install

## Debian

On Debian or Ubuntu, the Passgen package can be installed with APT.

```bash
wget "https://passgen.gitlab.io/passgen/nightly/passgen-linux-amd64.deb"
wget "https://passgen.gitlab.io/passgen/nightly/passgen-linux-amd64.deb.sig"
wget "https://passgen.gitlab.io/passgen/passgen.pub"
ssh-keygen -Y verify -f passgen.pub -s passgen-linux-amd64.deb.sig -n file -I nightly@passgen.it < passgen-linux-amd64.deb
sudo apt install ./passgen-linux-amd64.deb
```

