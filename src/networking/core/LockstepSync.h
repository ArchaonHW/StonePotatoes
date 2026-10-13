// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Networking/LockstepSync.{h,cpp} (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine Networking/LockstepSync.{h,cpp}（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace StonePotatoes {
namespace Networking {

/**
 * LockstepSync — deterministic lockstep command synchronization.
 *
 * RTS/simulation sync model: all peers execute the same command set at the same tick
 * for deterministic replay. Each tick, every peer broadcasts a command packet (may be empty).
 * The local peer advances tick T only after receiving command packets from all peers for T.
 *
 * inputDelayTicks: local inputs are queued for tick currentTick+delay, leaving RTT time.
 *   ticks < delay are treated as empty commands from all peers (all peers must use the same
 *   numPeers/inputDelayTicks, otherwise sync semantics are not guaranteed).
 *
 * Typical per-frame flow:
 *   sync.QueueLocalInput(bytes);            // 0..n times
 *   for (auto& pkt : sync.SealLocalPackets()) transport.send(pkt);
 *   while (recv) sync.OnPeerPacket(bytes);  // receive remote packets
 *   TickCommands tc;
 *   while (sync.TryAdvance(tc)) sim(tc);    // advance if ready
 *
 * This layer does NOT include reliability/retransmission — UDP packet loss causes stall
 * (ReadyToAdvance = false). Upper layer decides whether to resend LastLocalPacket or
 * handle disconnect.
 * LockstepSync — 決定性 lockstep 指令同步。
 * 所有 peer 在相同 tick 執行相同指令集即可決定性重演。每 tick 各端廣播一個指令包。
 * 本端收齊所有 peer 對 tick T 的指令包才准推進 T。
 * inputDelayTicks：本地輸入排到 currentTick+delay 生效，留網路往返時間。
 * 本層不含可靠性/重傳——UDP 掉包會造成 stall，由上層決定重送或斷線處理。
 */
class LockstepSync {
public:
    struct Config {
        int numPeers = 2;
        int localPeerId = 0;
        int inputDelayTicks = 2;
    };

    struct TickCommands {
        int tick = 0;
        // (peerId, blob) list, blob = one command from that peer for this tick
        std::vector<std::pair<int, std::vector<uint8_t>>> inputs;
    };

    explicit LockstepSync(const Config& cfg);

    // Queue one local input into the current tick window (packed into SealLocalPackets)
    void QueueLocalInput(const std::vector<uint8_t>& data);
    void QueueLocalInput(const std::string& data) {
        QueueLocalInput(std::vector<uint8_t>(data.begin(), data.end()));
    }

    // Seal local input packets. Fill gaps between lastSealed+1 and currentTick+delay
    // with empty packets; the last packet carries the queued inputs. Returns wire packets
    // to broadcast (normally one per tick, may be multiple on tick skip).
    std::vector<std::vector<uint8_t>> SealLocalPackets();

    // Receive a remote wire packet (command or checksum; auto-dispatched by magic).
    // Invalid format/illegal peerId/tick < delay command packets are discarded (return false).
    bool OnPeerPacket(const std::vector<uint8_t>& packet);

    // Do all peers have a command packet for currentTick?
    bool ReadyToAdvance() const;

    // Pop currentTick command set and advance. Returns false if not ready (out unchanged).
    bool TryAdvance(TickCommands& out);

    int CurrentTick() const { return m_CurrentTick; }
    // Highest tick seen from each peer (stall diagnostics)
    int PeerMaxTick(int peerId) const;

    // ---- Deterministic verification (desync detection) ----
    // Record local state checksum at tick (CRC32 or custom hash)
    void RecordLocalChecksum(int tick, uint32_t crc);
    // Generate checksum wire packet for broadcast
    static std::vector<uint8_t> EncodeChecksumPacket(int peerId, int tick, uint32_t crc);
    // Compare checksums at tick across peers. Returns empty if all match (or insufficient data).
    std::vector<int> ChecksumMismatches(int tick) const;

    // Wire format:
    //   command: 'L''K' | ver(1) | peerId(1) | tick(4 LE) | n(2 LE) | [len(2 LE) | bytes]×n
    //   checksum: 'C''K' | ver(1) | peerId(1) | tick(4 LE) | crc(4 LE)
    static std::vector<uint8_t> EncodeCommandPacket(
        int peerId, int tick, const std::vector<std::vector<uint8_t>>& blobs);

private:
    static constexpr uint8_t kWireVersion = 1;

    bool DecodeCommandPacket(const std::vector<uint8_t>& pkt);
    bool DecodeChecksumPacket(const std::vector<uint8_t>& pkt);

    Config m_Cfg;
    int m_CurrentTick = 0;
    int m_LastSealedTick;  // last tick we sealed locally

    // peer -> tick -> command blob list for that tick
    std::vector<std::map<int, std::vector<std::vector<uint8_t>>>> m_Bundles;
    std::vector<int> m_PeerMaxTick;
    std::vector<std::vector<uint8_t>> m_PendingLocal;  // inputs waiting to be sealed

    // peer -> tick -> crc
    std::vector<std::map<int, uint32_t>> m_Checksums;
};

} // namespace Networking
} // namespace StonePotatoes
