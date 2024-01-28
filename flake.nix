{
  description = "A clipboard manager with gpg support for wayland";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      libs = with nixpkgs.legacyPackages.x86_64-linux; [
        meson
        pkg-config
        cmake
        ninja
        boost
        msgpack-cxx
        gpgme
        libgpg-error
        magic-enum
        procps
      ];
    in
    {
      packages.x86_64-linux = rec {
        default = wlclipmgr;
        wlclipmgr = pkgs.stdenv.mkDerivation {
          name = "wlclipmgr";
          src = ./.;

          nativeBuildInputs = libs;
          buildPhase = ''
            mkdir -p build
            meson setup build $src --buildtype release
            meson compile -C build
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp build/wlclipmgr $out/bin
          '';
        };
      };

      devShells.x86_64-linux.default = pkgs.mkShell {
        nativeBuildInputs = libs;
      };
  };
}
