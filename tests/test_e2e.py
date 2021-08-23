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
def mounted_elm_ap(elm_ap_image_file, program_path, tmp_path, llvm_coverage):
    yield from mounted_image(program_path, elm_ap_image_file, tmp_path)


@pytest.fixture
def mounted_elm_ec(elm_ec_image_file, program_path, tmp_path, llvm_coverage):
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


def test_file_not_found(mounted_elm_ec):
    path = mounted_elm_ec / "blah" / "not" / "here"
    assert not path.exists()


def test_fmap_name(mounted_elm_ap):
    assert (mounted_elm_ap / "name").read_text() == "FMAP\n"


AP_REGIONS = [
    "COREBOOT",
    "FMAP",
    "FW_MAIN_A",
    "FW_MAIN_B",
    "GBB",
    "RO_FRID",
    "RO_SECTION",
    "RO_VPD",
    "RW_ELOG",
    "RW_FWID_A",
    "RW_FWID_B",
    "RW_LEGACY",
    "RW_NVRAM",
    "RW_SECTION_A",
    "RW_SECTION_B",
    "RW_SHARED",
    "RW_VPD",
    "SHARED_DATA",
    "VBLOCK_A",
    "VBLOCK_B",
    "WP_RO",
]


def test_fmap_regions(mounted_elm_ap):
    regions_from_fs = [d.name for d in (mounted_elm_ap / "areas").iterdir()]
    regions_from_fs.sort()
    assert AP_REGIONS == regions_from_fs


def test_raw_write_empty(mounted_elm_ap):
    raw_file = mounted_elm_ap / "areas" / "RO_FRID" / "raw"
    orig_data = raw_file.read_bytes()
    assert raw_file.stat().st_size == 256

    raw_file.write_bytes(b"")
    assert raw_file.stat().st_size == 256
    assert raw_file.read_bytes() == orig_data


def test_raw_write_small(mounted_elm_ap):
    raw_file = mounted_elm_ap / "areas" / "RO_FRID" / "raw"
    orig_data = raw_file.read_bytes()
    assert raw_file.stat().st_size == 256
    assert len(orig_data) == 256

    data = bytes([0xDE, 0xAD, 0xBE, 0xEF])
    raw_file.write_bytes(data)
    assert raw_file.stat().st_size == 256

    new_data = data + orig_data[4:]
    assert raw_file.read_bytes() == new_data


def test_raw_write_exact(mounted_elm_ap):
    raw_file = mounted_elm_ap / "areas" / "RO_FRID" / "raw"
    assert raw_file.stat().st_size == 256

    data = bytes([0xDE, 0xAD, 0xBE, 0xEF] * 64)
    raw_file.write_bytes(data)
    assert raw_file.stat().st_size == 256
    assert raw_file.read_bytes() == data


def test_raw_write_too_big(mounted_elm_ap):
    raw_file = mounted_elm_ap / "areas" / "RO_FRID" / "raw"
    assert raw_file.stat().st_size == 256

    data = bytes([0xDE, 0xAD, 0xBE, 0xEF] * 64 + [0xAA])
    with pytest.raises(OSError):
        raw_file.write_bytes(data)
    assert raw_file.stat().st_size == 256
    assert raw_file.read_bytes() == data[:-1]


def test_gbb_hwid(mounted_elm_ap):
    hwid_file = mounted_elm_ap / "areas" / "GBB" / "gbb-data" / "hwid"
    assert hwid_file.read_text() == "ELM A1B-C2D-A3A\n"


def test_gbb_hwid_edit(mounted_elm_ap):
    hwid_file = mounted_elm_ap / "areas" / "GBB" / "gbb-data" / "hwid"
    new_hwid = "ELM-ZZCR C3B-A4D-D1A-D5F\n"
    hwid_file.write_text(new_hwid)
    assert hwid_file.read_text() == new_hwid

    new_hwid_no_newline = new_hwid.rstrip()
    gbb_raw_file = mounted_elm_ap / "areas" / "GBB" / "raw"
    assert new_hwid_no_newline.encode("utf-8") in gbb_raw_file.read_bytes()
