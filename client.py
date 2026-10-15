import sys
import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 8080
MAX_WORKERS = 100

values = [
    1987654,
    123456,
    1456789,
    321654,
    987654,
    543210,
    1122334,
    789012,
    1567890,
    456789,
    1890123,
    234567,
    1678901,
    654321,
    1345678,
    876543,
    1550000,
    345678,
    1789012,
    567890,
    1290123,
    432109,
    1901234,
    765432,
    1112223,
    3600000,
    1432109,
    500001,
    1999999,
    1000001,
    1500000,
    200000,
    900001,
    1750000,
    1250000,
    800000,
    1600000,
    300000,
    1300000,
    600000,
    1100000,
    400000,
    1800000,
    700000,
    1400000,
    500000,
    1000000,
    2000000,
    1350000,
    275000,
    1450000,
    975000,
    1850000,
    650000,
    2100000,
    420000,
    1320000,
    880000,
    1555000,
    610000,
    1700000,
    720000,
    1420000,
    510000,
    1950000,
    820000,
    1200000,
    460000,
    1650000,
    930000,
    1380000,
    580000,
    2050000,
    340000,
    1480000,
    990000,
    1120000,
    680000,
    1760000,
    390000,
    1520000,
    860000,
    1280000,
    540000,
    1880000,
    760000,
    1180000,
    430000,
    1620000,
    950000,
    1340000,
    620000,
    2150000,
    370000,
    1460000,
    980000,
    1080000,
    700000,
    1820000,
    410000,
]

count = min(int(sys.argv[1]) if len(sys.argv) > 1 else 50, MAX_WORKERS)

print(f"Client • {count} Aufrufe → {HOST}:{PORT}\n")
print("[JobID] Time (Input) | Result")
start_total = time.perf_counter()
lock = threading.Lock()


def worker(i):
    value = values[i - 1]

    start = time.perf_counter()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(f"{value}\n".encode())
        data = s.recv(1024)

    duration = time.perf_counter() - start

    with lock:
        print(f"[ {i:>3} ] {duration:.4f}s | ({value}) → {data.decode().strip()}")


threads = []

for i in range(1, count + 1):
    t = threading.Thread(target=worker, args=(i,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

total = time.perf_counter() - start_total

print(f"\nGesamtzeit: {total:.4f}s")
