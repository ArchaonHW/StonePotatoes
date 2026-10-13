// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine Networking/LockstepSync.cpp (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine Networking/LockstepSync.cpp（MIT），詳見 THIRD_PARTY_NOTICES.md。
#include "LockstepSync.h"

#include <cstring>

namespace StonePotatoes {
namespace Networking {

namespace {

void PutU16(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
}

void PutU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
}

bool GetU16(const uint8_t*& p, const uint8_t* end, uint16_t& x) {
    if (end - p < 2) return false;
    x = static_cast<uint16_t>(p[0] | (p[1] << 8));
    p += 2;
    return true;
}

bool GetU32(const uint8_t*& p, const uint8_t* end, uint32_t& x) {
    if (end - p < 4) return false;
    x = static_cast<uint32_t>(p[0]) | (p[1] << 8) |
        (static_cast<uint32_t>(p[2]) << 16) |
        (static_cast<uint32_t>(p[3]) << 24);
    p += 4;
    return true;
}

} // namespace

LockstepSync::LockstepSync(const Config& c) : m_Cfg(c) {
    if (m_Cfg.numPeers < 1) m_Cfg.numPeers = 1;
    if (m_Cfg.numPeers > 255) m_Cfg.numPeers = 255;
    if (m_Cfg.localPeerId < 0 || m_Cfg.localPeerId >= m_Cfg.numPeers)
        m_Cfg.localPeerId = 0;
    if (m_Cfg.inputDelayTicks < 1) m_Cfg.inputDelayTicks = 1;

    m_Bundles.resize(m_Cfg.numPeers);
    m_PeerMaxTick.assign(m_Cfg.numPeers, -1);
    m_Checksums.resize(m_Cfg.numPeers);
    m_LastSealedTick = m_Cfg.inputDelayTicks - 1;

    // ticks < delay are always empty commands from all peers — pre-fill
    for (int t = 0; t < m_Cfg.inputDelayTicks; ++t) {
        for (auto& m : m_Bundles) m[t] = {};
    }
}

void LockstepSync::QueueLocalInput(const std::vector<uint8_t>& data) {
    m_PendingLocal.push_back(data);
}

std::vector<uint8_t> LockstepSync::EncodeCommandPacket(
    int peerId, int tick, const std::vector<std::vector<uint8_t>>& blobs) {
    std::vector<uint8_t> pkt;
    pkt.reserve(9 + blobs.size() * 2);
    pkt.push_back('L'); pkt.push_back('K');
    pkt.push_back(kWireVersion);
    pkt.push_back(static_cast<uint8_t>(peerId));
    PutU32(pkt, static_cast<uint32_t>(tick));
    PutU16(pkt, static_cast<uint16_t>(blobs.size()));
    for (const auto& b : blobs) {
        PutU16(pkt, static_cast<uint16_t>(b.size()));
        pkt.insert(pkt.end(), b.begin(), b.end());
    }
    return pkt;
}

std::vector<uint8_t> LockstepSync::EncodeChecksumPacket(int peerId, int tick, uint32_t crc) {
    std::vector<uint8_t> pkt;
    pkt.reserve(10);
    pkt.push_back('C'); pkt.push_back('K');
    pkt.push_back(kWireVersion);
    pkt.push_back(static_cast<uint8_t>(peerId));
    PutU32(pkt, static_cast<uint32_t>(tick));
    PutU32(pkt, crc);
    return pkt;
}

std::vector<std::vector<uint8_t>> LockstepSync::SealLocalPackets() {
    std::vector<std::vector<uint8_t>> out;
    const int target = m_CurrentTick + m_Cfg.inputDelayTicks;
    while (m_LastSealedTick < target) {
        const int t = ++m_LastSealedTick;
        const bool last = (m_LastSealedTick == target);
        auto& blobList = m_Bundles[m_Cfg.localPeerId][t];
        if (last) {
            blobList = m_PendingLocal;
            m_PendingLocal.clear();
        }
        out.push_back(EncodeCommandPacket(m_Cfg.localPeerId, t, blobList));
        if (t > m_PeerMaxTick[m_Cfg.localPeerId]) {
            m_PeerMaxTick[m_Cfg.localPeerId] = t;
        }
    }
    return out;
}

bool LockstepSync::OnPeerPacket(const std::vector<uint8_t>& pkt) {
    if (pkt.size() < 4) return false;
    if (pkt[0] == 'L' && pkt[1] == 'K') return DecodeCommandPacket(pkt);
    if (pkt[0] == 'C' && pkt[1] == 'K') return DecodeChecksumPacket(pkt);
    return false;
}

bool LockstepSync::DecodeCommandPacket(const std::vector<uint8_t>& pkt) {
    const uint8_t* p = pkt.data() + 2;
    const uint8_t* end = pkt.data() + pkt.size();
    if (*p++ != kWireVersion) return false;
    const int peerId = *p++;
    if (peerId < 0 || peerId >= m_Cfg.numPeers || peerId == m_Cfg.localPeerId) {
        return false;  // local packet should not come from network
    }
    uint32_t tick;
    uint16_t n;
    if (!GetU32(p, end, tick) || !GetU16(p, end, n)) return false;
    if (static_cast<int>(tick) < m_Cfg.inputDelayTicks) return false;
    if (static_cast<int>(tick) < m_CurrentTick) return false;

    std::vector<std::vector<uint8_t>> blobs;
    blobs.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        uint16_t len;
        if (!GetU16(p, end, len) || end - p < len) return false;
        blobs.emplace_back(p, p + len);
        p += len;
    }
    m_Bundles[peerId][static_cast<int>(tick)] = std::move(blobs);
    if (static_cast<int>(tick) > m_PeerMaxTick[peerId]) {
        m_PeerMaxTick[peerId] = static_cast<int>(tick);
    }
    return true;
}

bool LockstepSync::DecodeChecksumPacket(const std::vector<uint8_t>& pkt) {
    if (pkt.size() != 12) return false;
    const uint8_t* p = pkt.data() + 2;
    const uint8_t* end = pkt.data() + pkt.size();
    if (*p++ != kWireVersion) return false;
    const int peerId = *p++;
    if (peerId < 0 || peerId >= m_Cfg.numPeers) return false;
    uint32_t tick, crc;
    if (!GetU32(p, end, tick) || !GetU32(p, end, crc)) return false;
    m_Checksums[peerId][static_cast<int>(tick)] = crc;
    return true;
}

bool LockstepSync::ReadyToAdvance() const {
    for (int peer = 0; peer < m_Cfg.numPeers; ++peer) {
        if (m_Bundles[peer].find(m_CurrentTick) == m_Bundles[peer].end()) {
            return false;
        }
    }
    return true;
}

bool LockstepSync::TryAdvance(TickCommands& out) {
    if (!ReadyToAdvance()) return false;
    out.tick = m_CurrentTick;
    out.inputs.clear();
    for (int peer = 0; peer < m_Cfg.numPeers; ++peer) {
        for (auto& blob : m_Bundles[peer][m_CurrentTick]) {
            out.inputs.emplace_back(peer, std::move(blob));
        }
        auto& m = m_Bundles[peer];
        m.erase(m.begin(), m.upper_bound(m_CurrentTick));
    }
    ++m_CurrentTick;
    return true;
}

int LockstepSync::PeerMaxTick(int peerId) const {
    if (peerId < 0 || peerId >= m_Cfg.numPeers) return -1;
    return m_PeerMaxTick[peerId];
}

void LockstepSync::RecordLocalChecksum(int tick, uint32_t crc) {
    m_Checksums[m_Cfg.localPeerId][tick] = crc;
}

std::vector<int> LockstepSync::ChecksumMismatches(int tick) const {
    std::vector<int> bad;
    const auto lit = m_Checksums[m_Cfg.localPeerId].find(tick);
    if (lit == m_Checksums[m_Cfg.localPeerId].end()) return bad;
    const uint32_t ref = lit->second;
    for (int peer = 0; peer < m_Cfg.numPeers; ++peer) {
        if (peer == m_Cfg.localPeerId) continue;
        const auto it = m_Checksums[peer].find(tick);
        if (it != m_Checksums[peer].end() && it->second != ref) {
            bad.push_back(peer);
        }
    }
    return bad;
}

} // namespace Networking
} // namespace StonePotatoes
