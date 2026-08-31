#!/usr/bin/env python3
import socket


def connect(host: str, port: int, timeout: float) -> socket.socket:
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.settimeout(timeout)
    return sock


def recv_exact(sock: socket.socket, size: int) -> bytes:
    """Receive exactly size bytes, or fail if the peer closes early."""
    data = bytearray()

    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise AssertionError(
                f"peer closed early: expected {size} bytes, "
                f"received {len(data)} bytes"
            )
        data.extend(chunk)

    return bytes(data)


def make_payload(size: int, seed: int = 0) -> bytes:
    """
    Generate deterministic, non-uniform bytes.

    Using non-uniform data makes ordering/corruption bugs easier to detect
    than a payload such as b'a' * size.
    """
    return bytes(((i + seed) % 251) for i in range(size))
