# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.
## Communication
- Always respond in English, even if the prompt is written in French.
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

LK8000 is a tactical flight computer for glider/paraglider pilots (GPL-2.0). It is a large, mostly C++ (C++17)
codebase that targets many platforms from one shared source tree: Windows CE PDAs (PPC2003), PNA GPS units,
Windows PC, Linux desktop, the Kobo e-reader (repurposed as a flight instrument), Raspberry Pi, and Android.
Most logic lives under `Common/Source` and `Common/Header` and is compiled differently per target rather than
duplicated.

## Build system

There are two separate, non-interchangeable build systems:

- **`Makefile`** (repo root) — builds every target except Android: `PPC2003`, `PNA`, `PC`, `PCX64`, `LINUX`, `KOBO`, `PI`.
- **`CMakeLists.txt`** (repo root) — builds only the Android target, producing a shared library (`libLK8000.so`)
  consumed by the Gradle project under `android/` and `android-studio/`.

### Makefile targets (desktop/embedded)

```sh
make TARGET=LINUX                 # build only
make TARGET=LINUX install         # build + install into $HOME/LK8000
make TARGET=PC distrib            # build + install into LK8000/Distrib/PC/LK8000
make TARGET=KOBO                  # cross-compile for Kobo (needs KOBO=<rootfs>, default /opt/kobo-rootfs)
make TARGET=PI                    # cross-compile for Raspberry Pi (needs PI=<rootfs> to cross-compile)
make TARGET=LINUX clean           # also wipes the Distrib folder
```

Common options (see `README.md` for the full list): `DEBUG=y|n`, and for `LINUX`:
`OPENGL=y|n`, `USE_EGL=y|n`, `GLES=y|n`, `GLES2=y|n`, `USE_SDL=y|n`, `ENABLE_MESA_KMS=y|n`,
`GREYSCALE=y DITHER=y` (to emulate the Kobo's greyscale panel), `FULLSCREEN=y|n`.

`./makeall` builds every non-Android target in sequence (PPC2003, PNA, PC, LINUX, KOBO) and is what CI-equivalent
full builds run; `.circleci/config.yml` does the same per-target builds nightly using the `lk8000/lk8000:build`
Docker image.

Toolchains for Kobo/WinCE cross-compiling: http://lk8000.it/toolchain/, or use the Docker image
`lk8000/lk8000:build`.

Useful extra Makefile targets: `make cppcheck` (runs `cppcheck --enable=all` over `$(SRC_FILES)`), `make tags`.

### CMake (Android)

Built via Gradle (`android-studio/gradlew`), which invokes CMake for the native `LK8000` shared library defined
in `CMakeLists.txt`. When adding a source file that must be compiled on Android, it needs to be added to the
`add_library(LK8000 SHARED ...)` file list in `CMakeLists.txt` in addition to `Makefile`'s `SRC_FILES`-style
variables — the two build systems do not share a file list and commonly drift out of sync when new files are added.

## Kobo toolchain (primary focus of the `buildroot` branch)

Building `TARGET=KOBO` needs two separate pieces of cross-compilation infrastructure, both external to the
`make` invocation itself:

1. **The cross-compiler toolchain** (`arm-kobo-linux-gnueabihf-*`) — built with crosstool-NG from
   `Scripts/kobo-ct-ng.config`, or downloaded prebuilt from http://lk8000.it/toolchain/. It must be on `$PATH`.
2. **The Kobo rootfs** (`$KOBO`, default `/opt/kobo-rootfs`) — a sysroot containing cross-built versions of the
   third-party libraries LK8000 links against on Kobo (zlib, libpng, boost, freetype, geographiclib, openssl,
   curl, zzip, jpeg, …). This is built by `Scripts/kobo-build-rootfs/`, not by the main `Makefile`.

`Scripts/dockerfile` documents the canonical end-to-end sequence (also what `lk8000/lk8000:build` bakes in):

```sh
# 1. install/extract the prebuilt cross-compiler and put it on PATH
tar -xJf arm-kobo-linux-gnueabihf-*.tar.xz -C /opt
export PATH=$PATH:/opt/arm-kobo-linux-gnueabihf/bin

# 2. cross-build the rootfs libraries into $KOBO (default /opt/kobo-rootfs)
cd Scripts/kobo-build-rootfs
./build-all.sh                 # runs scripts/<lib>.sh in a fixed dependency order
export KOBO=/opt/kobo-rootfs

# 3. build LK8000 itself against that rootfs
make TARGET=KOBO
```

Inside `Scripts/kobo-build-rootfs/`:
- **`build-config.sh`** centralizes settings (`DEVICEROOT`/`KOBO` install path, `CROSSTARGET=arm-kobo-linux-gnueabihf`,
  `ARCHIVESDIR`/`PATCHESDIR`, `MAKE_JOBS`, …). Override values via a gitignored `build-config-user.sh` in the same
  directory rather than editing `build-config.sh` directly.
- **`build-all.sh`** builds each `scripts/<lib>.sh` in sequence and stops on first failure; skip specific libs with
  `SKIP="lib1 lib2" ./build-all.sh`. Each `scripts/<lib>.sh` fetches/patches/builds one dependency and marks itself
  done under `status/` (via `build-common.sh`, sourced by every per-lib script) so re-running `build-all.sh` skips
  already-built libs — delete the relevant `status/<lib>` file (or the whole build dir) to force a rebuild.
- **`arm-kobo-linux-gnueabihf.cmake`** is the CMake toolchain file for any of those dependencies that use CMake
  instead of autotools.
- Current upstream library set is `zlib, libpng, boost, freetype, geographiclib, openssl, curl, zzip, jpeg` — this
  is the base to extend when adding a new rootfs dependency (e.g. Bluetooth/BLE-related libs such as `bluez`,
  `dbus`, `glib`, `gattlib`, `expat`, `libffi`, `pcre`, `alsa`, `libsndfile`, `iconv` needed for BLE instrument
  support): add a new `scripts/<lib>.sh` following the existing ones' structure and append it to the `for i in ...`
  list in `build-all.sh`.

### `buildroot/`: an in-repo Buildroot SDK instead of crosstool-NG

`buildroot/` (repo root) is a self-contained Buildroot **external tree** that produces a Kobo cross-toolchain SDK
plus the extra target libraries LK8000 needs, as an alternative to the crosstool-NG toolchain +
`Scripts/kobo-build-rootfs/` flow above. It is *not* a git submodule and does not depend on any sibling project —
Buildroot itself is fetched on demand by `buildroot/getbuildroot.sh` (a hash-verified release-tarball download,
same pattern as the `getbuildroot.sh` in the F5OEO/tezuka_fw project), into `buildroot/buildroot/` (gitignored).
It was bootstrapped from the (separate, sibling) `buildroot-kobo` project's `buildroot-external/` tree, trimmed
down to just what's needed to compile LK8000 — no kernel/rootfs-image/secure-boot/wifi parts, which live only in
that other project.

Setup and build:

```sh
cd buildroot
./getbuildroot.sh                        # fetches buildroot/buildroot/ (skips if already present)
source sourceme.first                    # sets BR2_EXTERNAL to this directory
cd buildroot
make BR2_EXTERNAL=$BR2_EXTERNAL kobo_defconfig
make BR2_EXTERNAL=$BR2_EXTERNAL host-environment-setup boost zziplib libpng jpeg freetype openssl libcurl geographiclib
```

Then, back in the LK8000 repo root:

```sh
source buildroot/buildroot/output/host/environment-setup
make TARGET=KOBO KOBO_SDK=y
```

`environment-setup` exports `CC`/`CXX`/`AR`/`LD`/`STRIP`/etc. directly, puts its `bin/` on `$PATH`, and exports
`STAGING_DIR` pointing at `buildroot/buildroot/output/host/arm-buildroot-linux-gnueabihf/sysroot`.
**`host-environment-setup` must be built explicitly** (as above) — it's a standalone host package, not a
dependency of the library packages, so building only e.g. `boost geographiclib ...` will not produce
`environment-setup` on its own; if it's stale/missing, `source`-ing it silently fails and leaves whatever
toolchain was already on `$PATH` in place.

`Makefile` is wired for this via the `KOBO_SDK` flag (default `n`, opt-in only — plain `make TARGET=KOBO` is
unaffected and still uses the crosstool-NG `arm-kobo-linux-gnueabihf-` prefix):
- When `TARGET_IS_KOBO` and `KOBO_SDK=y`, `TCPATH` is left empty and `CC`/`CXX`/`AS`/`STRIP`/`AR` are **not**
  derived from `$(TCPATH)` at all — the same `USE_ENV_TOOLCHAIN` mechanism `TARGET=OPENVARIO` already used, just
  extended to also cover this case (Makefile, "tools" section) — so whatever `environment-setup` exported is used
  as-is. `SIZE` is the one exception: Buildroot-generated `environment-setup` scripts never export it (it's not an
  autotools variable), so the `Makefile` falls back to the host's own `size` (`SIZE ?= size`) — safe, since `size`
  just parses ELF section headers and doesn't care about the target architecture.
- `build/pkgconfig.mk` and the KOBO `LDFLAGS` block (`--rpath-link`) each add `$(STAGING_DIR)/usr/lib[/pkgconfig]`
  alongside `$(KOBO)` when `KOBO_SDK=y`, so libraries in the Buildroot sysroot resolve automatically.
- `build/kobo.mk`'s device-runtime-library bundling (the libs copied to `/opt/LK8000/lib` on the Kobo) also
  branches on `KOBO_SDK`: `KOBO_EXTRA_LIB_DIR` is `$(STAGING_DIR)/usr/lib` instead of `$(KOBO)/lib`, and
  `KOBO_SYS_LIB_PATHS` searches both `$(SYSROOT)/lib` and `$(SYSROOT)/usr/lib` (different toolchains split libc
  vs. libstdc++ across those differently — Bootlin's keeps `libstdc++.so.6` under `usr/lib`). The candidate-name
  list also includes Buildroot zziplib's `libzzip-0.so.N`/`libzzipmmapped-0.so.N` naming (vs. the plain
  `libzzip.so.N` from the autotools build `Scripts/kobo-build-rootfs` produces) — both variants are listed and
  whichever actually exists is kept.

`buildroot/configs/kobo_defconfig` enables a prebuilt **Bootlin `armv7-eabihf glibc bleeding-edge` toolchain**
(`BR2_TOOLCHAIN_EXTERNAL_BOOTLIN` + `BR2_TOOLCHAIN_EXTERNAL_BOOTLIN_ARMV7_EABIHF_GLIBC_BLEEDING_EDGE`, GCC 14.2.0 —
same pattern as `BR2_TOOLCHAIN_EXTERNAL_BOOTLIN_*` in the F5OEO/tezuka_fw project's board defconfigs), rather than
Kobo's original vendor Linaro 4.9.4 toolchain, which can't compile LK8000 (it requires `-std=c++20`, unsupported
by GCC 4.9). Using a modern, unrelated-vendor glibc here is safe only *because* LK8000 ships its own
glibc/libstdc++/ld.so to `/opt/LK8000/lib` on the device (the `TARGET_IS_KOBO` `LDFLAGS` block in the main
`Makefile`) instead of relying on the stock Kobo firmware's runtime — the app's own bundled runtime just needs to
match this toolchain, not the device's stock userspace. Plus the libraries LK8000 needs beyond what that
toolchain's own sysroot provides:
- `boost`, `zziplib`, `libpng`, `jpeg`/`jpeg-turbo`, `freetype`, `openssl`, `libcurl` are all mainline Buildroot
  packages. `boost` with no `BOOST_*` sub-options selected only installs headers (no compiled libs built) —
  sufficient for LK8000, which only uses header-only `boost::intrusive`. `libcurl`'s TLS backend is a separate
  choice (default `TLS_NONE`) — `BR2_PACKAGE_OPENSSL` + `BR2_PACKAGE_LIBCURL_OPENSSL` must both be set to actually
  get `libssl`/`libcrypto` and HTTPS support.
- **`geographiclib` has no mainline Buildroot package**, so `buildroot/package/geographiclib/` (`Config.in` +
  `geographiclib.mk` + `geographiclib.hash`, registered via `buildroot/Config.in`) is a custom one, modeled on
  mainline's `package/proj/` (another CMake-based geo C++ lib) for the `cmake-package` infra pattern. Version
  2.5.1 (matching `Scripts/kobo-build-rootfs/scripts/geographiclib.sh`), fetched from the same SourceForge release
  tarball.
- Building any of these installs into `buildroot/buildroot/output/host/arm-buildroot-linux-gnueabihf/sysroot`,
  i.e. `$(STAGING_DIR)`. `Scripts/kobo-build-rootfs/` is untouched and still exists as the crosstool-NG-toolchain
  alternative.
- Changing a package's Buildroot config option (e.g. adding a TLS backend) doesn't trigger a rebuild by itself —
  Buildroot tracks "already built" per-package via stamp files, not by config hash. Force one with `make
  BR2_EXTERNAL=$BR2_EXTERNAL <pkg>-dirclean <pkg>` after `kobo_defconfig`.

**Verified on this machine, full real build:** `source buildroot/buildroot/output/host/environment-setup && make
TARGET=KOBO KOBO_SDK=y` compiles every translation unit in LK8000 (including the bundled `Library/poco`) with GCC
14.2.0 and produces working `Kobo-install.zip`/`Kobo-install-otg.zip`, with `Bin/KOBO/std/KoboRoot/opt/LK8000/lib/`
correctly populated (libc/libstdc++ runtime plus freetype/png/curl/ssl/crypto/zzip/geographiclib). Two unrelated
unmet-dependency gotchas were hit and fixed along the way, worth knowing about if they resurface:
- `lib/json` (nlohmann/json) and `lib/fifo_map` are git submodules too (alongside `lib/doctest`/`lib/glm`); if
  uninitialized, compilation fails with `fatal error: nlohmann/json.hpp: No such file or directory` — fixed by
  `git submodule update --init --recursive`.
- `xsltproc` (XML dialog minifier, listed in `README.md`'s Raspberry Pi deps and `Scripts/dockerfile`) is a
  **host** package needed to build the dialog resources — unrelated to the target toolchain.

A newer glibc/libc header set than before now also declares `strlcat`/`strlcpy` itself, producing harmless
`-Wredundant-decls` warnings against LK8000's own compat declarations in
`Common/Source/Topology/shapelib/mapserver.h` — cosmetic, not a build failure.

Separately, `kobo/` (repo root) holds *device-side* rootfs files (`inittab`, `rcS`, udev rules, kernel modules) —
what runs on the Kobo itself — as opposed to `Scripts/kobo-build-rootfs/`, which is *host-side* tooling to produce
the cross-compilation sysroot. Don't confuse the two when the task is about "the Kobo rootfs."

### On-device debugging: USB gadget networking (telnet/ftp)

For iterating on a real device without pulling the SD card or reinstalling `KoboRoot.tgz` each time, `kobo/rcS`
already sources `/mnt/onboard/LK8000/kobo/init.sh` on every boot if that file exists — a safe, fully reversible
extension point (delete the file to revert) that needs no rebuild/reflash otherwise. `kobo/debug-network-init.sh`
is such a script: it brings up a USB Ethernet gadget (always `192.168.2.1` on the device side) with `telnetd`
(root shell, no login), `ftpd -A` (full filesystem access, no login), and a `dnsmasq` DHCP server so the PC side
gets an address automatically — no manual `ip addr add` needed, it just shows up once you assign the interface an
address via DHCP (NetworkManager does this on its own for most new wired-style interfaces). `telnetd`/`ftpd`
mirror the existing `KoboExportSerial()`/`KoboUnexportSerial()` pattern in `Common/Source/xcs/Kobo/System.cpp`,
just for `g_ether` instead of `g_serial`. `dnsmasq` is bundled from the Buildroot SDK (stock Kobo busybox has
neither `udhcpd` nor `zcip`) and, since it wasn't linked with a custom `--dynamic-linker`, is launched via our own
bundled `ld.so` + `LD_LIBRARY_PATH` the same way `LK8000-KOBO` uses `--dynamic-linker`/`--rpath` for the same
reason — see the script for the exact invocation.

**Never ship this to end users** — it's an unauthenticated root backdoor over USB. It's opt-in, off by default:

```sh
make TARGET=KOBO KOBO_SDK=y KOBO_DEBUG_NET=y     # bakes it into KoboRoot.tgz as .../LK8000/kobo/init.sh
```

(`build/kobo.mk`'s `build_distrib_kobo` installs it conditionally — a *shell-level* `if`, not a Make `ifeq`, since
the macro is expanded via `$(call ...)` inside a recipe, where literal `ifeq`/`endif` text would just get passed
to the shell verbatim and fail; `KOBO_DEBUG_NET=y` currently requires `KOBO_SDK=y`, since `dnsmasq` only exists via
the Buildroot SDK's `output/target` — see `KOBO_DNSMASQ_BIN` in `build/kobo.mk`, which is *not* under
`$(STAGING_DIR)` since that's the cross-compilation sysroot for libraries, not where Buildroot installs
applications.) For a device that's already installed, without rebuilding, just copy `kobo/debug-network-init.sh`
onto the FAT32 partition (visible as a normal USB drive) as `LK8000/kobo/init.sh` directly (plus `dnsmasq` itself
to `/opt/LK8000/bin/` if it isn't already there) — same effect, no `KOBO_DEBUG_NET` needed. (`arcotg_udc`/
`g_ether` module paths and the platform driver directory — `/drivers/<platform>/usb/gadget/`, symlinked to
`/drivers/current` by `rcS` — vary by Kobo model; adjust if `insmod` fails, check `LK8000/kobo/init.log`.)

After rebooting with that in place, on the host PC — just wait for the new interface (`ip link show`, e.g.
`enx<mac>`) to pick up a DHCP lease (usually automatic), then:

```sh
telnet 192.168.2.1                        # root shell -- always this address, regardless of the PC's own DHCP IP
curl -T LK8000-KOBO ftp://192.168.2.1/opt/LK8000/bin/LK8000-KOBO   # push a file
```

The fast iterate loop this enables, without ever touching `rcS`/`inittab`/`KoboRoot.tgz`:

```sh
make TARGET=KOBO KOBO_SDK=y LK8000-KOBO                              # build just the binary
telnet 192.168.2.1  # kill -9 $(pidof LK8000-KOBO)                   # stop it (file must not be busy to overwrite)
curl -T LK8000-KOBO ftp://192.168.2.1/opt/LK8000/bin/LK8000-KOBO     # push it
telnet 192.168.2.1  # /opt/LK8000/bin/LK8000-KOBO                    # relaunch in the foreground, watch output live
```

`ftpd` needs the file it's overwriting to not be currently running (`ETXTBSY` → curl error 25/553), hence killing
first. This same push mechanism is also how to get updated `.so` files onto the device (e.g. after a `KOBO_SDK`
library rebuild) without a full `KoboRoot.tgz` reinstall — push each one to `/opt/LK8000/lib/`.

## Tests

There is no separate test runner/binary for the main app. Unit tests are written with **doctest**
(`lib/doctest`, a git submodule — run `git submodule update --init` if it's missing) using `TEST_CASE(...)`
blocks placed directly inside the relevant implementation `.cpp` file (e.g. `Common/Source/Library/StringFunctions.cpp`,
`Common/Source/utils/md5.cpp`, `Common/Source/Calc/Radio.cpp`). Grep for `TEST_CASE` to find existing examples of
the convention before adding new tests.

The doctest runner is wired into `main()` in `Common/Source/lk8000.cpp`: unless `DOCTEST_CONFIG_DISABLE` is
defined, the compiled binary runs all registered doctest cases first (output redirected into LK8000's own log via
`startup_store_ostream`), and exits without starting the app if doctest decides it should exit (e.g. `--exit`,
`--list-test-cases`, or a failure with default settings). So testing a change to shared/library code is typically:
build a `LINUX` binary, then run it with doctest CLI flags, e.g.:

```sh
make TARGET=LINUX
./Bin/LINUX/LK8000-LINUX --test-case="*StringFunctions*" --exit
```

`Common/Utils/TestContest` is a legacy standalone WinCE-era test harness for the OLC/contest-scoring engine
(`ContestMgr`), with its own small `Makefile`; it's unrelated to doctest. `Common/Utils/test_tools` builds small
standalone simulator/debug binaries (`lxnano-sim`, `krt2-ping`, `flarm-sim`) for exercising specific device
protocols against real or simulated hardware, also unrelated to the doctest suite.

## Code style

`.clang-format` is present at the repo root (Chromium base style, C++11-labelled but the project builds as
C++17, 120-column limit, left-aligned pointers). Run `clang-format` on touched files before committing.

## Architecture notes

- **Platform abstraction is by preprocessor define, not by subclassing a single interface.** Code branches on
  target macros (`WINDOWSPC`, `PNA`, `KOBO`, `ANDROID`, `LINUX`, `GNAV`, etc., set by the active build system) far
  more than on runtime polymorphism. When editing shared code, check how existing `#if`/`#ifdef` blocks in the
  same file handle each target before adding a platform-specific branch.
- **`Common/Source/xcs/`** is a compatibility/porting layer (Android JNI wrappers, event/threading shims) mostly
  relevant to the Android build; native Android glue lives in `Common/Source/Android/`.
- **`Common/Source/Screen/` and `Common/Source/Window/`** implement LK8000's own portable UI toolkit
  (windows, controls, drawing) used across all targets instead of relying on each platform's native widget set.
- **`Common/Source/Calc/`** holds the flight/glide computer logic (McCready, task/AAT calculations, contest
  scoring via `ContestMgr`, wind, thermal analysis) — this is the core domain logic and is largely
  platform-independent.
- **`Common/Source/Devices/` and `Common/Source/Comm/`** handle instrument/GPS/FLARM device protocols and serial
  /Bluetooth/IOIO communication.
- **`Common/Source/Dialogs/`** contains the UI dialog screens; XML-ish resource/layout data backing them lives
  under `Common/Source/Resource` and `Common/Data`.
- **`kobo/`** contains target-device-specific boot/init files (rootfs `inittab`, `rcS`, udev rules, kernel
  modules) for turning a Kobo e-reader into the flight computer appliance — separate concern from the
  `TARGET=KOBO` app build itself, which is handled by `Makefile` + `Scripts/kobo-build-rootfs/`.
- **`lib/doctest`, `lib/glm`, `lib/json`, `lib/fifo_map`** are git submodules (`.gitmodules`); if they appear
  empty, run `git submodule update --init --recursive`.

## Git
- Do not mention Claude as author or co-author in commit messages (no
  "Co-Authored-By: Claude" trailer, no "Generated with Claude Code" line).
- Stage only files you changed. Commit/push only when asked.
- ASCII, no embedded double-quotes