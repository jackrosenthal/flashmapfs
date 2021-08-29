import subprocess

import pytest


@pytest.mark.parametrize(
    ["help_arg"],
    [
        ("--help",),
        ("-h",),
    ],
)
def test_help(program_path, help_arg):
    result = subprocess.run(
        [program_path, help_arg],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        timeout=5,
        encoding="utf-8",
    )
    assert result.stdout.startswith("Usage: ")
    assert result.returncode == 0


@pytest.mark.parametrize(
    ["args"],
    [
        ([],),
        (["foo.bin"],),
        (["foo.bin", "bar.bin", "baz.bin"],),
    ],
)
def test_bad_args(program_path, args):
    result = subprocess.run(
        [program_path, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        timeout=5,
        encoding="utf-8",
    )
    assert result.stdout.startswith("Usage: ")
    assert result.returncode == 1


def test_option_args(program_path):
    result = subprocess.run(
        [program_path, "-o", "allow_root", "-o", "allow_unmount", "--help"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        timeout=5,
        encoding="utf-8",
    )
    assert result.stdout.startswith("Usage: ")
    assert result.returncode == 0


@pytest.mark.parametrize(
    ["flash_contents"],
    [
        (b"",),
        (b"\xff",),
        # (b"__FMAP",),  # FIXME: segfault
        (b"__FMAP__",),
        (b"\xff\xff\xff\xff\xff\xff\xff\xff__FMAP__",),
        (
            b"__FMAP__\x01\x01\0\0\0\0\0\0\0\0\xff\xff\xff\xff"
            b"FIRMWARE_NAME_12345678912345678\0\x01\0"
            b"\x08\0\0\0\xff\xff\0\0FMAP_AREA_TOO_BIG_1234567890\0\0\0\0\0\0\0\0",
        ),
    ],
)
def test_bad_fmap(program_path, flash_contents, tmp_path):
    flash_file = tmp_path / "flash.bin"
    flash_file.write_bytes(flash_contents)
    mnt = tmp_path / "mount"
    mnt.mkdir()

    result = subprocess.run(
        [program_path, flash_file, mnt],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        timeout=5,
        encoding="utf-8",
    )
    assert result.returncode == 2


def test_image_does_not_exist(program_path, tmp_path):
    mnt = tmp_path / "mount"
    mnt.mkdir()

    result = subprocess.run(
        [program_path, "/file/does/not/exist.bin", mnt],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        timeout=5,
        encoding="utf-8",
    )
    assert result.returncode == 2
