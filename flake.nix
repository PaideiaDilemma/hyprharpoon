{
  description = "Hyprland Harpoon";

  inputs.hyprland.url = "github:hyprwm/Hyprland";

  outputs = inputs: let
    inherit (inputs.hyprland.inputs) nixpkgs;
    withPkgsFor = fn: nixpkgs.lib.genAttrs (builtins.attrNames inputs.hyprland.packages) (system: fn system nixpkgs.legacyPackages.${system});
  in {
    packages = withPkgsFor (system: pkgs: let
      hyprland = inputs.hyprland.packages.${system}.hyprland;
      buildPlugin = path: extraArgs:
        pkgs.callPackage path {
          inherit hyprland;
          inherit (hyprland) stdenv;
        }
        // extraArgs;
    in {
      hyprharpoon = buildPlugin ./hyprharpoon.nix {};
    });

    checks = withPkgsFor (system: pkgs: inputs.self.packages.${system});

    devShells = withPkgsFor (system: pkgs: {
      default = pkgs.mkShell.override {stdenv = pkgs.gcc13Stdenv;} {
        name = "hyprland-plugins";
        buildInputs = [inputs.hyprland.packages.${system}.hyprland pkgs.clang-tools pkgs.cmake];
        inputsFrom = [inputs.hyprland.packages.${system}.hyprland];
      };
    });
  };
}
