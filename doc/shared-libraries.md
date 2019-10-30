Shared Libraries
================

## galaxycashconsensus

The purpose of this library is to make the verification functionality that is critical to Bitcoin's consensus available to other applications, e.g. to language bindings.

### API

The interface is defined in the C header `galaxycashconsensus.h` located in  `src/script/galaxycashconsensus.h`.

#### Version

`galaxycashconsensus_version` returns an `unsigned int` with the API version *(currently at an experimental `0`)*.

#### Script Validation

`galaxycashconsensus_verify_script` returns an `int` with the status of the verification. It will be `1` if the input script correctly spends the previous output `scriptPubKey`.

##### Parameters
- `const unsigned char *scriptPubKey` - The previous output script that encumbers spending.
- `unsigned int scriptPubKeyLen` - The number of bytes for the `scriptPubKey`.
- `const unsigned char *txTo` - The transaction with the input that is spending the previous output.
- `unsigned int txToLen` - The number of bytes for the `txTo`.
- `unsigned int nIn` - The index of the input in `txTo` that spends the `scriptPubKey`.
- `unsigned int flags` - The script validation flags *(see below)*.
- `galaxycashconsensus_error* err` - Will have the error/success code for the operation *(see below)*.

##### Script Flags
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_NONE`
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_P2SH` - Evaluate P2SH ([BIP16](https://github.com/galaxycash/bips/blob/master/bip-0016.mediawiki)) subscripts
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_DERSIG` - Enforce strict DER ([BIP66](https://github.com/galaxycash/bips/blob/master/bip-0066.mediawiki)) compliance
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY` - Enforce NULLDUMMY ([BIP147](https://github.com/galaxycash/bips/blob/master/bip-0147.mediawiki))
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY` - Enable CHECKLOCKTIMEVERIFY ([BIP65](https://github.com/galaxycash/bips/blob/master/bip-0065.mediawiki))
- `galaxycashconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY` - Enable CHECKSEQUENCEVERIFY ([BIP112](https://github.com/galaxycash/bips/blob/master/bip-0112.mediawiki))

##### Errors
- `galaxycashconsensus_ERR_OK` - No errors with input parameters *(see the return value of `galaxycashconsensus_verify_script` for the verification status)*
- `galaxycashconsensus_ERR_TX_INDEX` - An invalid index for `txTo`
- `galaxycashconsensus_ERR_TX_SIZE_MISMATCH` - `txToLen` did not match with the size of `txTo`
- `galaxycashconsensus_ERR_DESERIALIZE` - An error deserializing `txTo`

### Example Implementations
- [NBitcoin](https://github.com/NicolasDorier/NBitcoin/blob/master/NBitcoin/Script.cs#L814) (.NET Bindings)
- [node-libgalaxycashconsensus](https://github.com/bitpay/node-libgalaxycashconsensus) (Node.js Bindings)
- [java-libgalaxycashconsensus](https://github.com/dexX7/java-libgalaxycashconsensus) (Java Bindings)
- [galaxycashconsensus-php](https://github.com/Bit-Wasp/galaxycashconsensus-php) (PHP Bindings)
