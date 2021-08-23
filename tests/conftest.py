import pathlib
import os
import subprocess

import pytest


def pytest_addoption(parser):
    here = pathlib.Path(__file__).parent
    parser.addoption(
        "--program",
        type=pathlib.Path,
        default=(here / ".." / "build" / "fmapfs").resolve(),
    )
    parser.addoption(
        "--llvm-coverage-out",
        type=pathlib.Path,
    )


@pytest.fixture(scope="session")
def program_path(request):
    return request.config.option.program


@pytest.fixture(scope="session")
def coverage_dir(request, tmp_path_factory):
    coverage_out = request.config.option.llvm_coverage_out
    if coverage_out:
        coverage_dir = tmp_path_factory.mktemp("coverage")
        yield coverage_dir

        # Merge coverage after tests finish
        subprocess.run(
            ["llvm-profdata", "merge", *coverage_dir.iterdir(), "-o", coverage_out],
            check=True,
        )
    else:
        yield


@pytest.fixture()
def llvm_coverage(coverage_dir):
    if coverage_dir:
        coverage_raw = coverage_dir / "coverage-%p.profraw"
        os.environ["LLVM_PROFILE_FILE"] = str(coverage_raw)
