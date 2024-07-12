# flashmapfs

This is a FUSE filesystem driver which implements mounting a [Flashmap]-based
firmware image as a filesystem.  Flashmap is a common firmware region data
structure format used by Coreboot, and can be found in Chromebook firmware
images.

## Building

To build, you'll need a system with `libfuse3` and associated development
headers, as well as the `meson` and `ninja` build systems.

On Arch Linux, you can install dependencies using:

```shellsession
# pacman -Syu --needed fuse3 meson ninja
```

On Debian/Ubuntu, you can install dependencies using:

```shellsession
# apt install libfuse3-dev meson ninja
```

Then, build using meson:

```shellsession
$ meson setup build
$ ninja -C build
```

The binary will be located at `build/fmapfs`.

## Usage

```shellsession
$ fmapfs <image_path> <mount_path>
```

The program will daemonize itself automatically.  To unmount, use
`umount <mount_path>`.

## Filesystem Layout

```
mountpoint
├── areas
│   ├── REGION_NAME
│   │   ├── compressed  # 0 or 1
│   │   ├── preserve    # 0 or 1
│   │   ├── raw         # The raw data in the region
│   │   ├── ro          # 0 or 1
│   │   └── static      # 0 or 1
│   ├── GBB
│   │   ├── compressed
│   │   ├── gbb-data    # Special format interpreter available for GBB
│   │   │   ├── flags   # GBB flag values
│   │   │   │   ├── default-boot-altfw       # 0 or 1
│   │   │   │   ├── dev-screen-short-delay   # ...
│   │   │   │   ├── disable-auxfw-software-sync
│   │   │   │   ├── disable-ec-software-sync
│   │   │   │   ├── disable-fw-rollback-check
│   │   │   │   ├── disable-fwmp
│   │   │   │   ├── disable-shutdown-on-lid-close
│   │   │   │   ├── enable-alternate-os
│   │   │   │   ├── enable-udc
│   │   │   │   ├── enter-triggers-tonorm
│   │   │   │   ├── force-dev-boot-altfw
│   │   │   │   ├── force-dev-boot-usb
│   │   │   │   ├── force-dev-mode
│   │   │   │   ├── force-manual-recovery
│   │   │   │   ├── load-option-roms
│   │   │   │   └── running-faft
│   │   │   └── hwid   # The HWID string
│   │   ├── preserve
│   │   ├── raw
│   │   ├── ro
│   │   └── static
│   └── ...
├── name      # The FMAP name
├── raw       # The raw FMAP data
└── version   # The FMAP version (e.g., "1.1")
```

## Examples

### General Regions

Update the contents of the `RO_FRID` region:

```shellsession
$ echo -n "Google_SomeModel" > areas/RO_FRID/raw
```

Set the preserve flag on a region:

```shellsession
$ echo 1 > areas/RW_VPD/preserve
```

### GBB Data

Show GBB flags and their values:

```shellsession
$ cd areas/GBB/gbb-data/flags
$ grep . *
default-boot-altfw:0
dev-screen-short-delay:1
disable-auxfw-software-sync:1
...
```

Set the `force-dev-mode` GBB flag:

```shellsession
$ echo 1 > areas/GBB/gbb-data/flags/force-dev-mode
```

Change the HWID:

```shellsession
$ echo "SOMEMODEL-ZZCR A1B-B2C-C3D-D4F" > areas/GBB/gbb-data/hwid
```

## Development

Tests are implemented using `pytest`.  Extra flags are available:

* `--program`: The path to the pre-built binary to test.
* `--llvm-coverage-out`: Path to write LLVM coverage data (optional).

Here's an example of building with coverage, running tests, and generating an
HTML coverage report:

```shellsession
$ CC=clang meson setup build -Db_coverage=true -Db_sanitize=address
$ ninja -C build
$ pytest --llvm-coverage-out build/cov.profdata
$ llvm-cov show -instr-profile build/cov.profdata -format=html -output-dir=build/htmlcov build/fmapfs
# Coverage wil be located at build/htmlcov/index.html
```

GitHub Actions has been set-up to automatically run unit tests and generate a
coverage report when a pull request is uploaded as well.

[Flashmap]: https://chromium.googlesource.com/chromiumos/third_party/flashmap/+/HEAD
