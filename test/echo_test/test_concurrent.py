#!/usr/bin/env python3
import argparse
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from common import connect, make_payload, recv_exact


def worker(
    worker_id: int,
    host: str,
    port: int,
    timeout: float,
    messages: int,
    payload_size: int,
) -> int:
    with connect(host, port, timeout) as sock:
        for message_id in range(messages):
            # Different content for every worker/message pair.
            seed = (worker_id * 37 + message_id * 17) % 251
            payload = make_payload(payload_size, seed=seed)

            sock.sendall(payload)
            echoed = recv_exact(sock, len(payload))

            if echoed != payload:
                raise AssertionError(
                    f"worker {worker_id}, message {message_id}: echo mismatch"
                )

    return worker_id


def main() -> None:
    parser = argparse.ArgumentParser(
        description="RomeSocket concurrent-client echo stress test."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9898)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument(
        "--clients",
        type=int,
        default=20,
        help="number of concurrent connections (default: 20)",
    )
    parser.add_argument(
        "--messages",
        type=int,
        default=100,
        help="messages per connection (default: 100)",
    )
    parser.add_argument(
        "--payload-size",
        type=int,
        default=1024,
        help="bytes per message (default: 1024)",
    )
    args = parser.parse_args()

    if args.clients <= 0 or args.messages <= 0 or args.payload_size <= 0:
        raise SystemExit("clients/messages/payload-size must all be > 0")

    start = time.perf_counter()

    with ThreadPoolExecutor(max_workers=args.clients) as pool:
        futures = [
            pool.submit(
                worker,
                worker_id,
                args.host,
                args.port,
                args.timeout,
                args.messages,
                args.payload_size,
            )
            for worker_id in range(args.clients)
        ]

        completed = 0
        for future in as_completed(futures):
            worker_id = future.result()
            completed += 1
            print(
                f"[PASS] client {worker_id:>3} finished "
                f"({completed}/{args.clients})"
            )

    elapsed = time.perf_counter() - start
    total_messages = args.clients * args.messages
    total_bytes = total_messages * args.payload_size

    print(
        f"\nAll concurrent tests passed: "
        f"{args.clients} clients, {total_messages:,} messages, "
        f"{total_bytes:,} bytes echoed in {elapsed:.3f}s."
    )


if __name__ == "__main__":
    main()
