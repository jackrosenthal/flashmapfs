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
