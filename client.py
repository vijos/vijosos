import random
import shutil
import socket
import struct


CLOCK_FREQ = 125000000.0
JUDGE_RESP_ACK = 1
JUDGE_RESP_RESULT = 2
RETRY_LIMIT = 3
SERVER = ('192.168.1.56', 1234)
TFTP_ROOT = 'tftp_root'
PACKET_LOSS = 0.0


def send_packet(sock, pkt):
    if random.random() < PACKET_LOSS:
        return 0
    return sock.send(pkt)


def recv_packet(sock, length):
    while True:
        pkt = sock.recv(length)
        if random.random() < PACKET_LOSS:
            continue
        else:
            return pkt


def do_judge(filename, stdin_file, time_limit, mem_limit):
    time_limit_cycles = int(time_limit * CLOCK_FREQ)

    shutil.copyfile(filename, TFTP_ROOT + '/elf')
    shutil.copyfile(stdin_file, TFTP_ROOT + '/stdin')

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.connect(SERVER)

    for i in range(RETRY_LIMIT):
        if i > 0:
            print('Retry', i, '...')
        seq = random.randint(0, (1 << 64) - 1)
        req = struct.pack('<QQQ', seq, time_limit_cycles, mem_limit)
        send_packet(sock, req)

        sock.settimeout(0.5)
        try:
            while True:
                ack = recv_packet(sock, 2 * 8)
                if len(ack) != 2 * 8:
                    continue
                ack_seq, flags = struct.unpack('<QQ', ack)
                if ack_seq != seq or (flags & JUDGE_RESP_ACK) == 0:
                    continue
                break
        except socket.timeout:
            continue

        sock.settimeout(time_limit * 1.1 + 10)
        try:
            while True:
                resp = recv_packet(sock, 6 * 8)
                if len(resp) != 6 * 8:
                    continue
                resp_seq, flags, error, exitcode, time_usage, mem_usage = struct.unpack('<QQqqQQ', resp)
                if resp_seq != seq or (flags & JUDGE_RESP_ACK) == 0 \
                   or (flags & JUDGE_RESP_RESULT) == 0:
                    continue
                break
        except socket.timeout:
            continue

        time_usage /= CLOCK_FREQ

        with open(TFTP_ROOT + '/stdout', 'rb') as f:
            stdout = f.read()
        return exitcode, time_usage, mem_usage, stdout
    return None, None, None, None


def benchmark(n):
    with open('result.csv', 'w') as f:  
        f.write('time (ns),memory (KiB)\n')
    for i in range(n):
        exitcode, time_usage, mem_usage, stdout = \
            do_judge('user/riscv/hello.elf', 'stdin.txt', 1.0, 256 * 1024)
        if time_usage is None:
            continue
        with open('result.csv', 'a') as f:
            f.write('{},{}\n'.format(int(time_usage * 1e9), mem_usage))


exitcode, time_usage, mem_usage, stdout = \
    do_judge('user/riscv/helloxx.elf', 'stdin.txt', 30.0, 256 * 1024)
if time_usage is not None:
    print('Result:', time_usage * 1e6, 'us,', mem_usage, 'KiB')
    print('Exit code:', exitcode)
    print('Output:')
    print(stdout.decode('UTF-8', 'ignore'))
else:
    print('Failed.')

#benchmark(10000)
