import os
import sys

# Ensure Python library path is accessible
sys.path.insert(0, os.path.abspath("../oppo gui/src"))

from oppo_control.protocol import OppoFrame
from oppo_control.commands import (
    encode_battery_read,
    encode_eq_read,
    encode_eq_write,
    encode_game_mode_read,
    encode_game_mode_write,
    encode_gestures_read,
    EQPreset
)

FIXTURES_DIR = os.path.abspath("fixtures")
os.makedirs(FIXTURES_DIR, tx_ok=True) if hasattr(os, 'tx_ok') else os.makedirs(FIXTURES_DIR, exist_ok=True)

fixtures = {
    # Requests
    "battery_query_req.bin": encode_battery_read(seq_id=0x01),
    "eq_query_req.bin": encode_eq_read(seq_id=0x02),
    "eq_write_req.bin": encode_eq_write(seq_id=0x03, preset=EQPreset.BASS),
    "game_mode_query_req.bin": encode_game_mode_read(seq_id=0x04),
    "game_mode_write_req.bin": encode_game_mode_write(seq_id=0x05, enable=True),
    "gestures_query_req.bin": encode_gestures_read(seq_id=0x06),

    # Captured / Synthetic Ground-Truth Responses
    # Binary Battery frame: Group 0x06, Cmd 0x01, Seq 0x01, Count=3 (Left=90%, Right=85%, Case=100% plugged in 0xE4)
    "battery_binary_res.bin": OppoFrame(
        group=0x06, cmd_id=0x01, seq_id=0x01,
        payload=bytes([0x00, 0x03, 0x01, 0x5A, 0x02, 0x55, 0x03, 0xE4])
    ).to_bytes(),

    # Push Notification Frame: Group 0x02, Cmd 0x04, Seq 0x10, Bud charging push
    "battery_push_res.bin": OppoFrame(
        group=0x02, cmd_id=0x04, seq_id=0x10,
        payload=bytes([0x00, 0x02, 0x01, 0x64, 0x02, 0x64])
    ).to_bytes(),

    # ASCII Battery frame: Group 0x06, Cmd 0x01, Seq 0x01, CSV "1,2,118,2,2,118,3,1,128"
    "battery_ascii_res.bin": OppoFrame(
        group=0x06, cmd_id=0x01, seq_id=0x01,
        payload=bytes([0x00, 0x00]) + b"1,2,118,2,2,118,3,1,128"
    ).to_bytes(),

    # EQ Response Frame: Group 0x0F, Cmd 0x01, Seq 0x02, payload 02 00 00 02 (Bass Boost)
    "eq_response_bass.bin": OppoFrame(
        group=0x0F, cmd_id=0x01, seq_id=0x02,
        payload=bytes([0x02, 0x00, 0x00, 0x02])
    ).to_bytes(),
}

for name, data in fixtures.items():
    filepath = os.path.join(FIXTURES_DIR, name)
    with open(filepath, "wb") as f:
        f.write(data)
    print(f"[+] Generated binary oracle fixture: {name} ({len(data)} bytes) -> {data.hex().upper()}")

print("[+] All binary oracle test fixtures generated successfully.")
