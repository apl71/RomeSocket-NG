#!/usr/bin/env python3
import argparse
import time

from common import connect, recv_exact


def test_repeated_connections(
    host: str,
    port: int,
    timeout: float,
    count: int,
) -> None:
    for i in range(count):
        payload = f"rome-connection-{i}".encode()

        with connect(host, port, timeout) as sock:
            sock.sendall(payload)
            echoed = recv_exact(sock, len(payload))
            if echoed != payload:
                raise AssertionError(f"echo mismatch on connection #{i}")

        if (i + 1) % 100 == 0 or i + 1 == count:
            print(f"[PASS] {i + 1:,}/{count:,} connect -> echo -> close")


def test_connect_and_close(
    host: str,
    port: int,
    timeout: float,
    count: int,
) -> None:
    """
    Exercise the server's recv()==0 path:
    connect and close without sending application data.
    """
    for i in range(count):
        sock = connect(host, port, timeout)
        sock.close()

    # Verify that the server still accepts connections afterward.
    payload = b"still-alive"
    with connect(host, port, timeout) as sock:
        sock.sendall(payload)
        echoed = recv_exact(sock, len(payload))

    if echoed != payload:
        raise AssertionError("server did not echo correctly after close-only test")

    print(f"[PASS] {count:,} connect -> immediate close")
    print("[PASS] server still accepts and echoes afterward")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="RomeSocket connection lifecycle stress test."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9898)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument(
        "--count",
        type=int,
        default=1000,
        help="number of normal connect/echo/close cycles (default: 1000)",
    )
    parser.add_argument(
        "--close-only-count",
        type=int,
        default=200,
        help="number of connect/immediate-close cycles (default: 200)",
    )
    args = parser.parse_args()

    if args.count <= 0 or args.close_only_count <= 0:
        raise SystemExit("counts must be > 0")

    start = time.perf_counter()

    test_repeated_connections(
        args.host, args.port, args.timeout, args.count
    )
    test_connect_and_close(
        args.host, args.port, args.timeout, args.close_only_count
    )

    elapsed = time.perf_counter() - start
    print(f"\nAll connection lifecycle tests passed in {elapsed:.3f}s.")


if __name__ == "__main__":
    main()
