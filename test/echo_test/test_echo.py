#!/usr/bin/env python3
import argparse
import time

from common import connect, make_payload, recv_exact


DEFAULT_SIZES = [
    1,
    5,
    1024,
    65535,
    65536,
    65537,
    256 * 1024 + 123,
]


def test_one(host: str, port: int, timeout: float, size: int) -> None:
    payload = make_payload(size, seed=size % 251)

    with connect(host, port, timeout) as sock:
        sock.sendall(payload)
        echoed = recv_exact(sock, len(payload))

    if echoed != payload:
        mismatch = next(
            (i for i, (a, b) in enumerate(zip(payload, echoed)) if a != b),
            None,
        )
        raise AssertionError(
            f"echo mismatch for {size} bytes"
            + (f", first mismatch at byte {mismatch}" if mismatch is not None else "")
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="RomeSocket basic echo/boundary test."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9898)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument(
        "--size",
        type=int,
        action="append",
        dest="sizes",
        help="payload size to test; may be specified multiple times",
    )
    args = parser.parse_args()

    sizes = args.sizes or DEFAULT_SIZES

    start = time.perf_counter()
    for size in sizes:
        if size <= 0:
            raise SystemExit("--size must be > 0")
        test_one(args.host, args.port, args.timeout, size)
        print(f"[PASS] echo {size:,} bytes")

    elapsed = time.perf_counter() - start
    print(f"\nAll basic echo tests passed in {elapsed:.3f}s.")


if __name__ == "__main__":
    main()
