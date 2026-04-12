{
  description = "Simple base flake for new environments";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
      # The unstable branch specifically for the latest CMake
    nixpkgs-unstable.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, nixpkgs-unstable, ... }: let
    system = "x86_64-linux";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs {
        inherit system;
      };
      unstable = import nixpkgs-unstable { inherit system; };

      stablePkgs = with pkgs;[
        ninja
        gdb
        clang
        # This pulls in the LLVM headers, libraries, and 'llvm-config'
      ];
      unstablePkgs = with unstable;[
        cmake
      ];
    in pkgs.mkShell {
        nativeBuildInputs = stablePkgs ++ unstablePkgs;
        buildInputs = [
          pkgs.llvm_18.dev
          pkgs.llvm_18.lib
          pkgs.zlib          # <--- Make sure this is here!
          pkgs.libffi
          pkgs.ncurses
        ];        
        packages = [
            pkgs.cloc
        ];
        shellHook = ''
            export LLVM_DIR=${pkgs.llvm_18.dev}/lib/cmake/llvm
             export CPATH="${pkgs.llvm_18.dev}/include:$CPATH"
            export LIBRARY_PATH="${pkgs.llvm_18.lib}/lib:$LIBRARY_PATH"
            export GDK_BACKEND=wayland
            echo "LLVM environment loaded!"
        '';
    };
  };
}
