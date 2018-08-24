// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>

#include <cassert>
#include <cstring>

CHKDF_HMAC_SHA256_L32::CHKDF_HMAC_SHA256_L32(const uint8_t *ikm, size_t ikmlen,
                                             const std::string &salt) {
    CHMAC_SHA256((const uint8_t *)salt.c_str(), salt.size())
        .Write(ikm, ikmlen)
        .Finalize(m_prk);
}

void CHKDF_HMAC_SHA256_L32::Expand32(const std::string &info,
                                     uint8_t hash[OUTPUT_SIZE]) {
    // expand a 32byte key (single round)
    assert(info.size() <= 128);
    static const uint8_t one[1] = {1};
    CHMAC_SHA256(m_prk, 32)
        .Write((const uint8_t *)info.data(), info.size())
        .Write(one, 1)
        .Finalize(hash);
}
