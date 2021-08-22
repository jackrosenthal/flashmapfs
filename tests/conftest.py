import pathlib

import pytest


def pytest_addoption(parser):
    here = pathlib.Path(__file__).parent
    parser.addoption(
        "--program",
        type=pathlib.Path,
        default=(here / ".." / "build" / "fmapfs").resolve(),
    )


@pytest.fixture(scope="session")
def program_path(request):
    return request.config.option.program
