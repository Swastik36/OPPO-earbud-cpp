#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>
#include "oppo/device.hpp"

#if defined(__linux__)
    #include <sys/prctl.h>
    #include <signal.h>
#endif

void print_usage() {
    std::cout << "OPPO Enco Buds3 Pro Linux Companion CLI & Sidecar Engine (C++ Native)\n\n"
              << "Usage:\n"
              << "  oppoctl-cpp [options] <command> [args]\n\n"
              << "Commands:\n"
              << "  stream               Launch bi-directional NDJSON streaming sidecar for Tauri\n"
              << "  info                 Query hardware info and firmware version\n"
              << "  battery              Query real-time battery status (Left, Right, Case)\n"
              << "  eq <preset>          Get or set EQ preset (original, clear_vocals, bass_boost)\n"
              << "  gamemode <on|off>    Get or set Low-Latency Game Mode\n"
              << "  gestures             Dump touch gestures configuration\n\n"
              << "Options:\n"
              << "  --mac <BDADDR>       Target Bluetooth MAC address (default: 60:55:56:22:49:A0)\n"
              << "  --channel <N>        RFCOMM Server Channel (default: 15)\n"
              << "  --trace              Enable raw TX/RX hex trace logging\n"
              << "  --help               Show this help menu\n";
}

void setup_process_death_guards() {
#if defined(__linux__)
    // 🛡️ Linux Kernel Parent Death Signal Guard:
    // Guarantees kernel sends SIGTERM if parent process (Tauri) exits or crashes.
    prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

    // 🛡️ Universal Cross-Platform Stdin Pipe Watcher (Linux, Windows, macOS):
    // Standard input closes automatically when the parent process exits.
    std::thread pipe_watcher([]() {
        while (std::cin.get() != EOF) {}
        std::exit(0);
    });
    pipe_watcher.detach();
}

int main(int argc, char* argv[]) {
    setup_process_death_guards();

    std::string mac_address = "60:55:56:22:49:A0";
    uint8_t channel = 15;
    bool trace = false;
    std::string command = "battery";
    std::string arg1 = "";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--mac" && i + 1 < argc) {
            mac_address = argv[++i];
        } else if (a == "--channel" && i + 1 < argc) {
            channel = static_cast<uint8_t>(std::stoi(argv[++i]));
        } else if (a == "--trace") {
            trace = true;
        } else if (a == "--help" || a == "-h") {
            print_usage();
            return 0;
        } else if (command == "battery" && !a.empty() && a[0] != '-') {
            command = a;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arg1 = argv[++i];
            }
        }
    }

    oppo::OppoDevice device;
    device.set_trace(trace);

    // Bi-directional Sidecar Streaming Mode for Tauri v2
    if (command == "stream") {
        std::mutex cout_mutex;
        auto emit_json = [&](const std::string& json_str) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << json_str << "\n" << std::flush;
        };

        emit_json("{\"type\":\"status\",\"state\":\"connecting\",\"mac\":\"" + mac_address + "\"}");

        // Register push notification NDJSON emitter
        device.register_push_callback([&emit_json](const oppo::OppoFrame&, const oppo::OppoMessage& msg) {
            if (std::holds_alternative<oppo::EarbudsBattery>(msg)) {
                const auto& b = std::get<oppo::EarbudsBattery>(msg);
                std::string json = "{\"type\":\"battery\",\"left\":" + std::to_string(b.left.percentage) +
                                   ",\"left_charging\":" + (b.left.is_charging ? "true" : "false") +
                                   ",\"right\":" + std::to_string(b.right.percentage) +
                                   ",\"right_charging\":" + (b.right.is_charging ? "true" : "false") +
                                   ",\"case\":" + std::to_string(b.case_val.percentage) +
                                   ",\"case_charging\":" + (b.case_val.is_charging ? "true" : "false") + "}";
                emit_json(json);
            }
        });

        auto conn_res = device.connect(mac_address, channel);
        if (!conn_res.has_value()) {
            emit_json("{\"type\":\"status\",\"state\":\"error\",\"message\":\"Failed to connect to device\"}");
            return 1;
        }

        emit_json("{\"type\":\"status\",\"state\":\"connected\",\"mac\":\"" + mac_address + "\"}");

        // Query startup battery status
        auto b_res = device.get_battery();
        if (b_res.has_value()) {
            const auto& b = b_res.value();
            std::string json = "{\"type\":\"battery\",\"left\":" + std::to_string(b.left.percentage) +
                               ",\"left_charging\":" + (b.left.is_charging ? "true" : "false") +
                               ",\"right\":" + std::to_string(b.right.percentage) +
                               ",\"right_charging\":" + (b.right.is_charging ? "true" : "false") +
                               ",\"case\":" + std::to_string(b.case_val.percentage) +
                               ",\"case_charging\":" + (b.case_val.is_charging ? "true" : "false") + "}";
            emit_json(json);
        }

        // Bi-directional command listener on stdin
        std::string input_line;
        while (std::getline(std::cin, input_line)) {
            if (input_line.find("set_eq") != std::string::npos) {
                oppo::EQPreset preset = oppo::EQPreset::ORIGINAL;
                if (input_line.find("vocals") != std::string::npos || input_line.find("\"preset\":1") != std::string::npos) {
                    preset = oppo::EQPreset::VOCALS;
                } else if (input_line.find("bass") != std::string::npos || input_line.find("\"preset\":2") != std::string::npos) {
                    preset = oppo::EQPreset::BASS;
                }
                if (device.set_eq(preset).has_value()) {
                    emit_json("{\"type\":\"eq\",\"preset\":" + std::to_string(static_cast<int>(preset)) + "}");
                }
            } else if (input_line.find("set_gamemode") != std::string::npos) {
                bool enable = (input_line.find("true") != std::string::npos || input_line.find("\"enable\":1") != std::string::npos);
                if (device.set_game_mode(enable).has_value()) {
                    emit_json("{\"type\":\"gamemode\",\"enabled\":" + std::string(enable ? "true" : "false") + "}");
                }
            } else if (input_line.find("get_battery") != std::string::npos) {
                auto bat = device.get_battery();
                if (bat.has_value()) {
                    const auto& b = bat.value();
                    std::string json = "{\"type\":\"battery\",\"left\":" + std::to_string(b.left.percentage) +
                                       ",\"left_charging\":" + (b.left.is_charging ? "true" : "false") +
                                       ",\"right\":" + std::to_string(b.right.percentage) +
                                       ",\"right_charging\":" + (b.right.is_charging ? "true" : "false") +
                                       ",\"case\":" + std::to_string(b.case_val.percentage) +
                                       ",\"case_charging\":" + (b.case_val.is_charging ? "true" : "false") + "}";
                    emit_json(json);
                }
            }
        }

        device.disconnect();
        return 0;
    }

    // Standard CLI Driver Mode
    std::cout << "Connecting to OPPO Earbuds [" << mac_address << "] on RFCOMM channel " << static_cast<int>(channel) << "...\n";

    device.register_push_callback([](const oppo::OppoFrame&, const oppo::OppoMessage& msg) {
        if (std::holds_alternative<oppo::EarbudsBattery>(msg)) {
            const auto& b = std::get<oppo::EarbudsBattery>(msg);
            std::cout << "[PUSH EVENT] Battery Update -> " << b.to_string() << "\n";
        }
    });

    auto conn_res = device.connect(mac_address, channel);
    if (!conn_res.has_value()) {
        std::cerr << "[-] Failed to connect to device: " << oppo::error_to_string(conn_res.error()) << "\n";
        return 1;
    }

    std::cout << "[+] Connected successfully.\n\n";

    if (command == "info") {
        auto fw_res = device.get_firmware();
        if (fw_res.has_value()) {
            std::cout << "Firmware Version: " << (fw_res.value().version.empty() ? "Unknown" : fw_res.value().version) << "\n";
        } else {
            std::cout << "[-] Failed to query firmware: " << oppo::error_to_string(fw_res.error()) << "\n";
        }
    } else if (command == "battery") {
        auto b_res = device.get_battery();
        if (b_res.has_value()) {
            const auto& b = b_res.value();
            std::cout << "Earbuds Battery Status:\n"
                      << "  Left Earbud:  " << b.left.percentage << "% [Charging: " << (b.left.is_charging ? "Yes" : "No") << "]\n"
                      << "  Right Earbud: " << b.right.percentage << "% [Charging: " << (b.right.is_charging ? "Yes" : "No") << "]\n"
                      << "  Case:         " << b.case_val.percentage << "% [Charging: " << (b.case_val.is_charging ? "Yes" : "No") << "]\n";
        } else {
            std::cout << "[-] Failed to query battery: " << oppo::error_to_string(b_res.error()) << "\n";
        }
    } else if (command == "eq") {
        if (arg1.empty()) {
            auto eq_res = device.get_eq();
            if (eq_res.has_value()) {
                std::cout << "Current EQ Preset: " << oppo::eq_preset_to_string(eq_res.value()) << "\n";
            } else {
                std::cout << "[-] Failed to query EQ: " << oppo::error_to_string(eq_res.error()) << "\n";
            }
        } else {
            oppo::EQPreset preset = oppo::EQPreset::ORIGINAL;
            std::string lower = arg1;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == "vocals" || lower == "clear_vocals" || lower == "1") preset = oppo::EQPreset::VOCALS;
            else if (lower == "bass" || lower == "bass_boost" || lower == "2") preset = oppo::EQPreset::BASS;

            auto set_res = device.set_eq(preset);
            if (set_res.has_value()) {
                std::cout << "[+] Updated EQ Preset to: " << oppo::eq_preset_to_string(preset) << "\n";
            } else {
                std::cout << "[-] Failed to set EQ: " << oppo::error_to_string(set_res.error()) << "\n";
            }
        }
    } else if (command == "gamemode") {
        if (arg1.empty()) {
            auto gm_res = device.get_game_mode();
            if (gm_res.has_value()) {
                std::cout << "Low-Latency Game Mode: " << (gm_res.value() ? "ON" : "OFF") << "\n";
            } else {
                std::cout << "[-] Failed to query Game Mode: " << oppo::error_to_string(gm_res.error()) << "\n";
            }
        } else {
            bool enable = (arg1 == "on" || arg1 == "1" || arg1 == "true");
            auto set_res = device.set_game_mode(enable);
            if (set_res.has_value()) {
                std::cout << "[+] Game Mode set to: " << (enable ? "ON" : "OFF") << "\n";
            } else {
                std::cout << "[-] Failed to set Game Mode: " << oppo::error_to_string(set_res.error()) << "\n";
            }
        }
    } else if (command == "gestures") {
        auto g_res = device.get_gestures();
        if (g_res.has_value()) {
            std::cout << "Touch Gestures Configuration:\n";
            for (const auto& g : g_res.value().gestures) {
                std::cout << "  - " << g.to_string() << "\n";
            }
        } else {
            std::cout << "[-] Failed to query gestures: " << oppo::error_to_string(g_res.error()) << "\n";
        }
    } else {
        std::cerr << "[-] Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }

    device.disconnect();
    return 0;
}
