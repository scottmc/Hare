To build, open up Terminal to this directory and
run "./build.sh". Afterwards, Hare and it's encoders will appear
built in the "dist" directory.

Don't run the Makefiles manually, as they don't work without the script.

Hare uses TagKit (https://github.com/scottmc/tagkit), which is a git submodule
in "src/tagkit". Clone with "git clone --recurse-submodules", or in an
existing clone run "git submodule update --init". "build.sh" does the latter
if it finds the folder empty. (A zip or tarball download from GitHub doesn't
include submodules, so it needs TagKit added by hand.)
