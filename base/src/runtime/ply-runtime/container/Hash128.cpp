/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

// Spooky Hash
// A 128-bit noncryptographic hash, for checksums and table lookup
// By Bob Jenkins.  Public domain.
//   Oct 31 2010: published framework, disclaimer ShortHash isn't right
//   Nov 7 2010: disabled ShortHash
//   Oct 31 2011: replace End, ShortMix, ShortEnd, enable ShortHash again
//   April 10 2012: buffer overflow on platforms without unaligned reads
//   July 12 2012: was passing out variables in final to in/out in short
//   July 30 2012: I reintroduced the buffer overflow
//   August 5 2012: SpookyV2: d = should be d += in short hash, and remove extra mix
//   from long hash

#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Hash128.h>
#include <string.h>

#define PLY_ALLOW_UNALIGNED_READS 1

namespace ply {

//
// short hash ... it could be used on any message,
// but it's used by Spooky just for short messages.
//
void SpookyHash::Short(const void* message, size_t length, u64* hash1, u64* hash2) {
    u64 buf[2 * sc_numVars];
    union {
        const u8* p8;
        u32* p32;
        u64* p64;
        size_t i;
    } u;

    u.p8 = (const u8*) message;

    if (!PLY_ALLOW_UNALIGNED_READS && (u.i & 0x7)) {
        memcpy(buf, message, length);
        u.p64 = buf;
    }

    size_t remainder = length % 32;
    u64 a = *hash1;
    u64 b = *hash2;
    u64 c = sc_const;
    u64 d = sc_const;

    if (length > 15) {
        const u64* end = u.p64 + (length / 32) * 4;

        // handle all complete sets of 32 bytes
        for (; u.p64 < end; u.p64 += 4) {
            c += u.p64[0];
            d += u.p64[1];
            ShortMix(a, b, c, d);
            a += u.p64[2];
            b += u.p64[3];
        }

        // Handle the case of 16+ remaining bytes.
        if (remainder >= 16) {
            c += u.p64[0];
            d += u.p64[1];
            ShortMix(a, b, c, d);
            u.p64 += 2;
            remainder -= 16;
        }
    }

    // Handle the last 0..15 bytes, and its length
    d += ((u64) length) << 56;
    switch (remainder) {
        case 15:
            d += ((u64) u.p8[14]) << 48;
        case 14:
            d += ((u64) u.p8[13]) << 40;
        case 13:
            d += ((u64) u.p8[12]) << 32;
        case 12:
            d += u.p32[2];
            c += u.p64[0];
            break;
        case 11:
            d += ((u64) u.p8[10]) << 16;
        case 10:
            d += ((u64) u.p8[9]) << 8;
        case 9:
            d += (u64) u.p8[8];
        case 8:
            c += u.p64[0];
            break;
        case 7:
            c += ((u64) u.p8[6]) << 48;
        case 6:
            c += ((u64) u.p8[5]) << 40;
        case 5:
            c += ((u64) u.p8[4]) << 32;
        case 4:
            c += u.p32[0];
            break;
        case 3:
            c += ((u64) u.p8[2]) << 16;
        case 2:
            c += ((u64) u.p8[1]) << 8;
        case 1:
            c += (u64) u.p8[0];
            break;
        case 0:
            c += sc_const;
            d += sc_const;
    }
    ShortEnd(a, b, c, d);
    *hash1 = a;
    *hash2 = b;
}

// do the whole hash in one call
void SpookyHash::Hash128(const void* message, size_t length, u64* hash1, u64* hash2) {
    if (length < sc_bufSize) {
        Short(message, length, hash1, hash2);
        return;
    }

    u64 h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11;
    u64 buf[sc_numVars];
    u64* end;
    union {
        const u8* p8;
        u64* p64;
        size_t i;
    } u;
    size_t remainder;

    h0 = h3 = h6 = h9 = *hash1;
    h1 = h4 = h7 = h10 = *hash2;
    h2 = h5 = h8 = h11 = sc_const;

    u.p8 = (const u8*) message;
    end = u.p64 + (length / sc_blockSize) * sc_numVars;

    // handle all whole sc_blockSize blocks of bytes
    if (PLY_ALLOW_UNALIGNED_READS || ((u.i & 0x7) == 0)) {
        while (u.p64 < end) {
            Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
            u.p64 += sc_numVars;
        }
    } else {
        while (u.p64 < end) {
            memcpy(buf, u.p64, sc_blockSize);
            Mix(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
            u.p64 += sc_numVars;
        }
    }

    // handle the last partial block of sc_blockSize bytes
    remainder = (length - ((const u8*) end - (const u8*) message));
    memcpy(buf, end, remainder);
    memset(((u8*) buf) + remainder, 0, sc_blockSize - remainder);
    ((u8*) buf)[sc_blockSize - 1] = (u8) remainder;

    // do some final mixing
    End(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
    *hash1 = h0;
    *hash2 = h1;
}

// init spooky state
void SpookyHash::Init(u64 seed1, u64 seed2) {
    m_length = 0;
    m_remainder = 0;
    m_state[0] = seed1;
    m_state[1] = seed2;
}

// add a message fragment to the state
void SpookyHash::Update(const void* message, size_t length) {
    u64 h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11;
    size_t new_length = length + m_remainder;
    u8 remainder;
    union {
        const u8* p8;
        u64* p64;
        size_t i;
    } u;
    const u64* end;

    // Is this message fragment too short?  If it is, stuff it away.
    if (new_length < sc_bufSize) {
        memcpy(&((u8*) m_data)[m_remainder], message, length);
        m_length = length + m_length;
        m_remainder = (u8) new_length;
        return;
    }

    // init the variables
    if (m_length < sc_bufSize) {
        h0 = h3 = h6 = h9 = m_state[0];
        h1 = h4 = h7 = h10 = m_state[1];
        h2 = h5 = h8 = h11 = sc_const;
    } else {
        h0 = m_state[0];
        h1 = m_state[1];
        h2 = m_state[2];
        h3 = m_state[3];
        h4 = m_state[4];
        h5 = m_state[5];
        h6 = m_state[6];
        h7 = m_state[7];
        h8 = m_state[8];
        h9 = m_state[9];
        h10 = m_state[10];
        h11 = m_state[11];
    }
    m_length = length + m_length;

    // if we've got anything stuffed away, use it now
    if (m_remainder) {
        u8 prefix = sc_bufSize - m_remainder;
        memcpy(&(((u8*) m_data)[m_remainder]), message, prefix);
        u.p64 = m_data;
        Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        Mix(&u.p64[sc_numVars], h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        u.p8 = ((const u8*) message) + prefix;
        length -= prefix;
    } else {
        u.p8 = (const u8*) message;
    }

    // handle all whole blocks of sc_blockSize bytes
    end = u.p64 + (length / sc_blockSize) * sc_numVars;
    remainder = (u8) (length - ((const u8*) end - u.p8));
    if (PLY_ALLOW_UNALIGNED_READS || (u.i & 0x7) == 0) {
        while (u.p64 < end) {
            Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
            u.p64 += sc_numVars;
        }
    } else {
        while (u.p64 < end) {
            memcpy(m_data, u.p8, sc_blockSize);
            Mix(m_data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
            u.p64 += sc_numVars;
        }
    }

    // stuff away the last few bytes
    m_remainder = remainder;
    memcpy(m_data, end, remainder);

    // stuff away the variables
    m_state[0] = h0;
    m_state[1] = h1;
    m_state[2] = h2;
    m_state[3] = h3;
    m_state[4] = h4;
    m_state[5] = h5;
    m_state[6] = h6;
    m_state[7] = h7;
    m_state[8] = h8;
    m_state[9] = h9;
    m_state[10] = h10;
    m_state[11] = h11;
}

// report the hash for the concatenation of all message fragments so far
void SpookyHash::Final(u64* hash1, u64* hash2) const {
    // init the variables
    if (m_length < sc_bufSize) {
        *hash1 = m_state[0];
        *hash2 = m_state[1];
        Short(m_data, m_length, hash1, hash2);
        return;
    }

    const u64* data = (const u64*) m_data;
    u8 remainder = m_remainder;

    u64 h0 = m_state[0];
    u64 h1 = m_state[1];
    u64 h2 = m_state[2];
    u64 h3 = m_state[3];
    u64 h4 = m_state[4];
    u64 h5 = m_state[5];
    u64 h6 = m_state[6];
    u64 h7 = m_state[7];
    u64 h8 = m_state[8];
    u64 h9 = m_state[9];
    u64 h10 = m_state[10];
    u64 h11 = m_state[11];

    if (remainder >= sc_blockSize) {
        // m_data can contain two blocks; handle any whole first block
        Mix(data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        data += sc_numVars;
        remainder -= sc_blockSize;
    }

    // mix in the last partial block, and the length mod sc_blockSize
    memset(&((u8*) data)[remainder], 0, (sc_blockSize - remainder));

    ((u8*) data)[sc_blockSize - 1] = remainder;

    // do some final mixing
    End(data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);

    *hash1 = h0;
    *hash2 = h1;
}

//------------------------------------------------------------

Hash128::Hash128() {
    this->state.Init(0, 0);
}

void Hash128::append(StringView view) {
    this->state.Update(view.bytes, view.num_bytes);
}

u128 Hash128::get() const {
    u128 r;
    this->state.Final(&r.hi, &r.lo);
    return r;
}

u128 Hash128::compute(StringView view) {
    Hash128 hasher;
    hasher.append(view);
    return hasher.get();
}

} // namespace ply
