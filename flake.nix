{
  description = "Random sequence generator based on regular expressions.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        passgen = pkgs.stdenv.mkDerivation {
          pname = "passgen";
          version = "1.0";
          src = ./.;
          nativeBuildInputs = [ pkgs.cmake pkgs.git ];
          cmakeFlags = [ "-DBUILD_GIT_INFO=OFF" ];
        };
      in {
        packages = {
          passgen = passgen;
          default = passgen;
        };
        devShell = with pkgs; mkShell { buildInputs = [ passgen ]; };
      });
}
