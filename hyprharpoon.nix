{
  lib,
  stdenv,
  hyprland,
}:
stdenv.mkDerivation {
  pname = "hyprharpoon";
  version = "0.1";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  buildInputs = [hyprland] ++ hyprland.buildInputs;

  meta = with lib; {
    homepage = "https://github.com/PaideiaDilemma/hyprharpoon";
    description = "Neovim harpoon but for hyprland windows";
    #license = licenses.todo;
    platforms = platforms.linux;
  };
}
