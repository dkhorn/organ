import socket
import mido

DEST_IP = "192.168.127.196"
DEST_PORT = 21928
MUDP_MAGIC = b"MU"
MUDP_VER = 1

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# pick the REAPER virtual MIDI output port name
PORT_NAME = "REAPER_MUDP_OUT 1"

with mido.open_input(PORT_NAME) as inp:
    batch = []
    for msg in inp:
        # Only channel messages (note/cc/etc). Skip sysex for now.
        if msg.type == "sysex":
            continue

        data = msg.bytes()  # includes status
        # We require full status bytes; mido already provides them.
        # Keep only 2 or 3 byte channel messages.
        if len(data) in (2, 3) and 0x80 <= data[0] <= 0xEF:
            batch.append(bytes(data))

        # Send immediately (or batch — this is “immediate”)
        if batch:
            # Header: 'M','U',ver,count
            pkt = bytearray()
            pkt += b"MUDP"  # we'll overwrite to MU + ver + count below for clarity
            pkt[0:2] = b"MU"
            pkt[2] = MUDP_VER
            pkt[3] = len(batch)
            for b in batch:
                pkt += b
            sock.sendto(pkt, (DEST_IP, DEST_PORT))
            batch.clear()
