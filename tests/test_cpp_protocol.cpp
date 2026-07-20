#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstring>
#include "oppo/protocol.hpp"
#include "oppo/messages.hpp"

std::vector<uint8_t> load_fixture(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Failed to open fixture file: " << path << "\n";
        exit(1);
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
}

void test_frame_serialization() {
    std::cout << "[TEST] 1. OppoFrame Serialization against Python Oracle...\n";
    oppo::OppoFrame frame(0x06, 0x01, 0x01, {});
    std::vector<uint8_t> encoded = frame.to_bytes();

    std::vector<uint8_t> expected = load_fixture("fixtures/battery_query_req.bin");
    assert(encoded == expected);
    std::cout << "  [PASS] Battery Query request matches Python oracle bytes 100%.\n";
}

void test_binary_battery_decoding() {
    std::cout << "[TEST] 2. Binary Battery Frame Decoding...\n";
    std::vector<uint8_t> raw = load_fixture("fixtures/battery_binary_res.bin");
    auto res = oppo::OppoFrame::from_bytes(raw);
    assert(res.has_value());

    oppo::OppoFrame frame = res.value();
    assert(frame.group == 0x06);
    assert(frame.cmd_id == 0x01);
    assert(frame.seq_id == 0x01);

    oppo::EarbudsBattery b = oppo::MessageCodec::parse_battery(frame);
    assert(b.left.percentage == 90);
    assert(b.left.is_charging == false);
    assert(b.right.percentage == 85);
    assert(b.right.is_charging == false);
    assert(b.case_val.percentage == 100);
    assert(b.case_val.is_charging == true); // Bit-7 charging check 0xE4 -> 0x64 (100%), charging=true
    std::cout << "  [PASS] Binary battery decoded correctly: " << b.to_string() << "\n";
}

void test_push_battery_decoding() {
    std::cout << "[TEST] 3. Unsolicited Push Battery Frame Decoding...\n";
    std::vector<uint8_t> raw = load_fixture("fixtures/battery_push_res.bin");
    auto res = oppo::OppoFrame::from_bytes(raw);
    assert(res.has_value());

    oppo::OppoFrame frame = res.value();
    assert(frame.group == 0x02);
    assert(frame.cmd_id == 0x04);

    oppo::EarbudsBattery b = oppo::MessageCodec::parse_battery(frame);
    assert(b.is_push_notification == true);
    assert(b.left.percentage == 100);
    assert(b.right.percentage == 100);
    std::cout << "  [PASS] Push battery notification correctly decoded with push flag.\n";
}

void test_stream_parser_junk_resync() {
    std::cout << "[TEST] 4. Stream Parser Resynchronization & Chunking...\n";
    oppo::OppoStreamParser parser;

    std::vector<uint8_t> valid_frame = load_fixture("fixtures/battery_query_req.bin");

    // Prepend 10 bytes of junk data before valid sync byte
    std::vector<uint8_t> noisy_stream = {0xFF, 0x00, 0x12, 0xAA, 0x55, 0x77, 0xBB, 0xCC, 0xDD, 0xEE};
    noisy_stream.insert(noisy_stream.end(), valid_frame.begin(), valid_frame.end());

    // Feed chunk 1
    auto frames1 = parser.feed(noisy_stream.data(), 12);
    assert(frames1.empty()); // Incomplete

    // Feed chunk 2 (rest of frame)
    auto frames2 = parser.feed(noisy_stream.data() + 12, noisy_stream.size() - 12);
    assert(frames2.size() == 1);
    assert(frames2[0].group == 0x06);
    assert(frames2[0].cmd_id == 0x01);
    std::cout << "  [PASS] Stream parser ignored junk and resynchronized cleanly on sync byte.\n";
}

void test_malformed_varint_handling() {
    std::cout << "[TEST] 5. LEB128 Varint Malformed MSB Guard...\n";
    // Sync byte 0xAA followed by 6 bytes with MSB=1 (0x80) to simulate malicious/corrupted stream
    std::vector<uint8_t> bad_varint = {0xAA, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00};
    auto res = oppo::OppoFrame::from_bytes(bad_varint);
    assert(!res.has_value());
    assert(res.error() == oppo::FrameError::MalformedVarint);
    std::cout << "  [PASS] LEB128 parser trapped 5-byte MSB limit without infinite loop or buffer overflow.\n";
}

int main() {
    std::cout << "Running OPPO C++ Protocol Native Test Suite...\n\n";
    test_frame_serialization();
    test_binary_battery_decoding();
    test_push_battery_decoding();
    test_stream_parser_junk_resync();
    test_malformed_varint_handling();
    std::cout << "\n[SUCCESS] ALL C++ NATIVE PROTOCOL TESTS PASSED CLEANLY (5/5).\n";
    return 0;
}
