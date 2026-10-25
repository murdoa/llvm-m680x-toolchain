# Compiler Explorer (godbolt.org), built from source. It is not in nixpkgs.
# The source comes from the `compiler-explorer` flake input; after bumping it
# with `nix flake update compiler-explorer`, update npmDepsHash too.
{ lib, buildNpmPackage, nodejs_22, makeWrapper, file, src }:

buildNpmPackage {
  pname = "compiler-explorer";
  version = "unstable-${src.lastModifiedDate}";
  inherit src;

  nodejs = nodejs_22;
  npmDepsHash = "sha256-A2DS8Qvnb9mZk5E9ly6L/8sQpmV5QQE6zjtJy5mr/bU=";

  nativeBuildInputs = [ makeWrapper ];

  # Cypress is only for CE's browser tests; don't let it download Electron.
  env.CYPRESS_INSTALL_BINARY = "0";
  env.HUSKY = "0";

  # CE runs /usr/bin/file to check binaries before executing them.
  postPatch = ''
    substituteInPlace lib/binaries/binary-utils.ts \
      --replace-fail /usr/bin/file ${lib.getExe file}
  '';

  # The same steps as `make prebuild`: bundle the front end with webpack and
  # compile the server to out/dist.
  buildPhase = ''
    runHook preBuild
    npm run webpack
    npm run ts-compile
    mkdir -p out/dist/etc/scripts/disasms
    cp -r etc/scripts/disasms/. out/dist/etc/scripts/disasms/
    echo ${src.rev or "unknown"} > out/dist/git_hash
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    share=$out/share/compiler-explorer
    mkdir -p $share
    # CE reads examples/, views/, etc/ and more relative to its root.
    cp -r . $share/
    runHook postInstall
  '';

  # compiler-explorer [--root-dir DIR] [--port N] ...
  # CE resolves some paths from the working directory, so run it from $share.
  postFixup = ''
    makeWrapper ${nodejs_22}/bin/node $out/bin/compiler-explorer \
      --chdir $out/share/compiler-explorer \
      --set NODE_ENV production \
      --add-flags "--no-warnings=ExperimentalWarning" \
      --add-flags "$out/share/compiler-explorer/out/dist/app.js" \
      --add-flags "--dist" \
      --add-flags "--static $out/share/compiler-explorer/out/webpack/static"
  '';

  meta = {
    description = "Interactively investigate compiler output";
    homepage = "https://github.com/compiler-explorer/compiler-explorer";
    license = lib.licenses.bsd2;
    mainProgram = "compiler-explorer";
  };
}
