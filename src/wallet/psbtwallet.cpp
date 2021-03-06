// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/error.h>
#include <wallet/psbtwallet.h>

TransactionError FillPSBT(const CWallet *pwallet,
                          PartiallySignedTransaction &psbtx, bool &complete,
                          SigHashType sighash_type, bool sign,
                          bool bip32derivs) {
    LOCK(pwallet->cs_wallet);
    // Get all of the previous transactions
    complete = true;
    for (size_t i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn &txin = psbtx.tx->vin[i];
        PSBTInput &input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Verify input looks sane.
        if (!input.IsSane()) {
            return TransactionError::INVALID_PSBT;
        }

        // If we have no utxo, grab it from the wallet.
        if (input.utxo.IsNull()) {
            const TxId &txid = txin.prevout.GetTxId();
            const auto it = pwallet->mapWallet.find(txid);
            if (it != pwallet->mapWallet.end()) {
                const CWalletTx &wtx = it->second;
                CTxOut utxo = wtx.tx->vout[txin.prevout.GetN()];
                // Update UTXOs from the wallet.
                input.utxo = utxo;
            }
        }

        // Get the Sighash type
        if (sign && input.sighash_type.getRawSigHashType() > 0 &&
            input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
        }

        // Get the scriptPubKey to know which SigningProvider to use
        CScript script;
        if (!input.utxo.IsNull()) {
            script = input.utxo.scriptPubKey;
        } else {
            // There's no UTXO so we can just skip this now
            complete = false;
            continue;
        }
        SignatureData sigdata;
        input.FillSignatureData(sigdata);
        const SigningProvider *provider =
            pwallet->GetSigningProvider(script, sigdata);
        if (!provider) {
            complete = false;
            continue;
        }

        complete &=
            SignPSBTInput(HidingSigningProvider(provider, !sign, !bip32derivs),
                          psbtx, i, sighash_type);
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that
    // hardware wallets can identify change
    for (size_t i = 0; i < psbtx.tx->vout.size(); ++i) {
        const CTxOut &out = psbtx.tx->vout.at(i);
        const SigningProvider *provider =
            pwallet->GetSigningProvider(out.scriptPubKey);
        if (provider) {
            UpdatePSBTOutput(
                HidingSigningProvider(provider, true, !bip32derivs), psbtx, i);
        }
    }

    return TransactionError::OK;
}
