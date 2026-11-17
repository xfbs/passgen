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
        # the version is defined in CMakeLists.txt, as `VERSION x.y.z`.
        version = builtins.head (builtins.match ".*VERSION ([0-9]+\\.[0-9]+\\.[0-9]+)\n.*"
          (builtins.readFile ./CMakeLists.txt));
        passgen = pkgs.stdenv.mkDerivation {
          pname = "passgen";
          inherit version;
          src = self;
          nativeBuildInputs = [ pkgs.cmake pkgs.git pkgs.clang ];
          cmakeFlags = [ "-DBUILD_GIT_INFO=OFF" ];
        };
        passgen-with-sanitizer = name:
          pkgs.clangStdenv.mkDerivation {
            pname = "passgen-${name}Sanitizier";
            inherit version;
            src = ./.;
            nativeBuildInputs = [ pkgs.cmake pkgs.git pkgs.ruby pkgs.bash ];
            buildPhase = ''
              make VERBOSE=1 -j
            '';
            cmakeFlags = [
              "-DBUILD_GIT_INFO=OFF"
              "-DCMAKE_BUILD_TYPE=Debug"
              "-DUSE_SANITIZER=${name}"
            ];
            checkPhase = ''
              ./src/tests/passgen-test
            '';
            doCheck = true;
          };
        passgen-asan = passgen-with-sanitizer "Address";
        passgen-ubsan = passgen-with-sanitizer "Undefined";
        passgen-msan = passgen-with-sanitizer "Memory";
        passgen-lsan = passgen-with-sanitizer "Leak";
      in {
        packages = {
          inherit passgen passgen-asan passgen-ubsan passgen-msan passgen-lsan;
          default = passgen;
        };
        checks = {
          inherit passgen passgen-asan passgen-ubsan passgen-msan passgen-lsan;
        };
        devShells.default = pkgs.mkShell {
          inputsFrom = [ passgen ];
          packages = [ pkgs.ninja pkgs.ruby pkgs.clang-tools ];
        };
      });
}
