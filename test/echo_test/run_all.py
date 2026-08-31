#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path


def run(script: Path, args: list[str]) -> None:
    print(f"\n========== {script.name} ==========", flush=True)
    subprocess.run(
        [sys.executable, str(script), *args],
        check=True,
        cwd=script.parent,
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run all RomeSocket black-box tests."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9898)
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    common_args = ["--host", args.host, "--port", str(args.port)]

    run(here / "test_echo.py", common_args)
    run(here / "test_connections.py", common_args)
    run(here / "test_concurrent.py", common_args)

    print("\n========== ALL TESTS PASSED ==========")


if __name__ == "__main__":
    main()
