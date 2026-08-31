# RomeSocket black-box tests

These tests use only the Python standard library. They treat the RomeSocket
EchoServer as a real TCP server, so they exercise the full path:

    TCP client
      -> accept
      -> io_uring recv
      -> RomeServer::on_recv()
      -> send
      -> io_uring send
      -> TCP client

## 1. Start EchoServer

Build RomeSocket and start the EchoServer in one terminal.

The current test defaults assume:

    host: 127.0.0.1
    port: 9898

For example:

    ./build/echo_server

Keep the server running while the Python tests execute.

## 2. Run all tests

In another terminal:

    cd romesocket_tests
    python3 run_all.py

If the server is elsewhere:

    python3 run_all.py --host 192.168.1.10 --port 9898

A non-zero exit or Python traceback means a test failed.

## 3. Run tests separately

### Basic echo / stream-boundary test

    python3 test_echo.py

It checks several payload sizes around the current 64 KiB RomeSocket buffer
boundary, plus a payload larger than one buffer.

Custom sizes:

    python3 test_echo.py --size 1 --size 65536 --size 1048576

The receiver deliberately loops until all expected bytes arrive. It does NOT
assume that one `send()` corresponds to one TCP `recv()`.

### Connection lifecycle test

    python3 test_connections.py

Defaults:
- 1000 connect -> echo -> close cycles
- 200 connect -> immediate-close cycles
- one final echo to verify that the server is still alive

Smaller quick run:

    python3 test_connections.py --count 100 --close-only-count 20

Heavier run:

    python3 test_connections.py --count 10000 --close-only-count 1000

### Concurrent-client test

    python3 test_concurrent.py

Defaults:
- 20 simultaneous TCP clients
- 100 request/echo rounds per client
- 1024-byte payload

Examples:

    python3 test_concurrent.py --clients 50 --messages 200
    python3 test_concurrent.py --clients 20 --messages 100 --payload-size 65536

## What these tests currently cover

- accept / new connections
- recv -> callback -> send
- TCP stream fragmentation/coalescing
- 64 KiB buffer boundary
- payloads larger than one RomeSocket receive buffer
- normal peer disconnect (`recv == 0`)
- repeated fd allocation/reuse
- multiple connections with interleaved io_uring completions

## What they do NOT force yet

They do not reliably force a partial TCP `send()` completion. Loopback is fast,
so the kernel will often accept the entire send at once.

A later dedicated test can deliberately shrink socket buffers and make the
client stop reading temporarily to exercise RomeServer's partial-send path.
