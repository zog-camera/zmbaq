{ pkgs ? import <nixpkgs> {} }:
let
  stdenv = pkgs.stdenv;
  fetchurl = pkgs.fetchurl;
  fetchFromGitHub = pkgs.fetchFromGitHub;
  Urho3D = stdenv.mkDerivation rec {
    version = "1.5";
    name = "Urho3D";
    src = fetchFromGitHub {
      owner = "urho3d";
      repo = "Urho3D";
      rev = "70db25ac459dd40f76580c8f6ee9320192a425ac";
    };
    cmakeFlags = cmakeFlags ++ [ "-DCMAKE_CXX_STANDARD=11 -DCMAKE_BUILD_TYPE=Debug" ];
    buildInputs = [ pkgs.cmake pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_ttf pkgs.glew pkgs.yasm ];
    enableParallelBuilding = true;
  };
in rec {
  ZOGProject = stdenv.mkDerivation {
    name = "ZOG";
  };
  nativeBuildInputs = [pkgs.cmake];
  buildInputs = [
    pkgs.boost
    pkgs.gtest
    pkgs.ffmpeg-full
    pkgs.SDL2
    pkgs.SDL2_image
    pkgs.SDL2_ttf
    pkgs.glew
    pkgs.zeromq
    Urho3D
  ];
  shellHook = [''
    echo Welcome to ZOG-project!
    echo Run \'mkdir build && cd build && cmake .. && make -j\' to build it.
  ''];
}
