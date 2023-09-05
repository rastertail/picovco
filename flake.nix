{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs { inherit system; };
    in {
      devShells.picovco = pkgs.mkShell {
        packages = with pkgs; [ git openssh bashInteractive gcc-arm-embedded cmake ninja python3 ];
      };

      apps.gen-dev-env = {
        type = "app";
        program = "${pkgs.writeScript "gen-dev-env.sh" ''
          #!/usr/bin/env bash

          # https://github.com/NixOS/nix/blob/a93110ab19085eeda1b4244fef49d18f91a1d7b8/src/nix/develop.cc#L257
          ignoreEnv=("BASHOPTS" "HOME" "NIX_BUILD_TOP" "NIX_ENFORCE_PURITY" "NIX_LOG_FD" "NIX_REMOTE" "PPID" "SHELL" "SHELLOPTS" "SSL_CERT_FILE" "TEMP" "TEMPDIR" "TERM" "TMP" "TMPDIR" "TZ" "UID")
          ignoreFilter=$(printf ",.key != \"%s\"" "''${ignoreEnv[@]}")

          nix print-dev-env --json $1 | ${pkgs.jq}/bin/jq -r "[.variables | to_entries | .[] | select(.value.type == \"exported\" and ([''${ignoreFilter:1}] | all)) | \"\(.key)=\" + @tsv \"\([.value.value])\"] | join(\"\n\")"
        ''}";
      };
    });
}