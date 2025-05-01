{
  description = "Brother QL-800 printer utility";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      # Support systems
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Import pkgs for each system
      pkgsFor = system: import nixpkgs { inherit system; };
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "brotherql800";
            version = "1.0.0";

            src = self;

            nativeBuildInputs = with pkgs; [
              cmake
              gcc
            ];

            buildInputs = with pkgs; [
              # Add runtime dependencies here
              libusb1
            ];

            configurePhase = ''
              cmake -B build .
            '';

            buildPhase = ''
              cmake --build build
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp build/brotherql800 $out/bin/
            '';
          };
        }
      );

      # Keep your existing devShell configuration
      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              cmake
              clang-tools
              gdb
              libusb1
              pkg-config
              # Add any other libraries your project needs
            ];

            hardeningDisable = [ "all" ];

            # This shellHook helps manage the build directory
            shellHook = ''
              echo "Welcome to the brotherql800 development environment!"
              echo "Run 'mkdir -p build && cd build && cmake .. && make' to build the project"
            '';
          };
        }
      );
    };
}
