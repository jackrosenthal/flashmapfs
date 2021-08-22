import lzma
import pathlib
import subprocess
import time

import pytest

HERE = pathlib.Path(__file__).parent


def decompress_image(path):
    with lzma.open(path) as f:
        return f.read()


@pytest.fixture(scope="session")
def elm_ap_image():
    return decompress_image(
        HERE / "data" / "bios-elm.ro-8438-140-0.rw-8438-184-0.bin.xz",
    )


@pytest.fixture(scope="session")
def elm_ec_image():
    return decompress_image(
        HERE / "data" / "ec-elm.ro-1-1-4818.rw-1-1-4824.bin.xz",
    )


@pytest.fixture
def elm_ap_image_file(elm_ap_image, tmp_path):
    out_file = tmp_path / "elm_ap.bin"
    out_file.write_bytes(elm_ap_image)
    return out_file


@pytest.fixture
def elm_ec_image_file(elm_ec_image, tmp_path):
    out_file = tmp_path / "elm_ec.bin"
    out_file.write_bytes(elm_ec_image)
    return out_file


def mounted_image(program_path, image_path, tmp_path):
    mountpoint = tmp_path / "mnt"
    mountpoint.mkdir()
    proc = subprocess.Popen(
        [program_path, "-f", image_path, mountpoint],
    )
    try:
        timeout = 5.0
        while not (mountpoint / "raw").exists() and timeout > 0:
            time.sleep(0.2)
            timeout -= 0.2
        yield mountpoint
    finally:
        if proc.poll() is not None:
            raise subprocess.SubprocessError(
                "FUSE process terminated prior to unmount!"
            )
        try:
            subprocess.run(["fusermount3", "-u", mountpoint], check=True)
            rv = proc.wait(timeout=5)
        finally:
            if proc.poll():
                proc.kill()
        if rv != 0:
            raise subprocess.SubprocessError(
                "FUSE process exited {} after unmount!".format(rv)
            )


@pytest.fixture
def mounted_elm_ap(elm_ap_image_file, program_path, tmp_path):
    yield from mounted_image(program_path, elm_ap_image_file, tmp_path)


@pytest.fixture
def mounted_elm_ec(elm_ec_image_file, program_path, tmp_path):
    yield from mounted_image(program_path, elm_ec_image_file, tmp_path)


def test_smoke_ap(mounted_elm_ap):
    pass


def test_smoke_ec(mounted_elm_ec):
    pass


def test_fmap_version_ap(mounted_elm_ap):
    assert (mounted_elm_ap / "version").read_text() == "1.0\n"


def test_edit_version(mounted_elm_ap):
    (mounted_elm_ap / "version").write_text("1.1")
    assert (mounted_elm_ap / "version").read_text() == "1.1\n"
