{ stdenv, pkgs }:
stdenv.mkDerivation {
  name = "passgen";

  buildInputs = [
    pkgs.utf8proc
    pkgs.libsodium
  ];

  nativeBuildInputs = [
    pkgs.pkgconfig
    pkgs.gnumake
    pkgs.meson
    pkgs.ninja
    pkgs.ruby
  ];

  src = ./.;

  installPhase = ''
    meson --prefix=$out build
    cd build
    ninja install
  '';
}
