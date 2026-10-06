#pragma once
#include <cstdint>
#include <functional>
#include <string>

struct NodeID {
    uint64_t lamport   = 0;
    uint32_t client_id = 0;
    uint32_t seq       = 0;

    constexpr auto operator<=>(const NodeID&) const = default;

    [[nodiscard]] std::string to_string() const {
        return "(" + std::to_string(lamport) + "," +
                     std::to_string(client_id) + "," +
                     std::to_string(seq) + ")";
    }

    static constexpr NodeID root() noexcept {
        return {0, 0, 0};
    }
};

template <>
struct std::hash<NodeID> {
    std::size_t operator()(const NodeID& id) const noexcept {
        std::size_t h = std::hash<uint64_t>{}(id.lamport);
        h ^= std::hash<uint32_t>{}(id.client_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(id.seq)       + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

/*
DESIGN NOTES — node_id.h
═════════════════════════════════════════════════════════════════════════════

WHAT THIS IS
────────────
Every character typed in the editor gets a NodeID — permanently. It is the
character's identity across every replica, for the lifetime of the document.
Once assigned, a NodeID is never changed and never reused.

FIELDS
──────
lamport    Logical clock. Incremented on every local edit and on every
           received remote edit. If operation A causally precedes B,
           A.lamport < B.lamport is guaranteed.

client_id  Stable unique ID for the originating replica. Assigned once at
           connection time. Breaks ties between concurrent ops from
           different users.

seq        Per-operation sequence counter. Disambiguates multiple characters
           produced in one logical tick — e.g. pasting "hello" creates five
           nodes that share the same lamport and client_id.

ORDERING & CONVERGENCE
──────────────────────
operator<=> with = default gives lexicographic comparison in field
declaration order: lamport → client_id → seq, all ascending. This is the
tie-breaking rule applied when two users insert at the same position
simultaneously. Because every replica runs the same deterministic comparison,
they all converge to the same document without coordination.

root()
──────
The invisible anchor node that sits before every real character. It is the
left-origin of the first insert. Never transmitted — every replica
synthesises it locally with {0, 0, 0}.

HASH
────
std::hash<NodeID> is specialised so NodeIDs can be used as keys in
unordered_map and unordered_set. Needed in Phase 2 for the causal buffer
(unordered_map<NodeID, PendingOp>). Uses FNV-inspired bit mixing — cheap
and sufficient for hash table use.

C++ STANDARD
────────────
operator<=> requires C++20. Set CMAKE_CXX_STANDARD to 20 in CMakeLists.txt.
*/