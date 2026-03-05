# pct_split_payment — Xahau Hook

A Xahau Hook that automatically forwards a configurable percentage of every incoming native XAH payment to a designated destination account.

## Overview

When an XAH payment arrives at the hook account, the hook calculates the configured percentage of the received amount and emits a new payment to a predefined destination. IOU payments and outgoing transactions are silently ignored.

## Hook Parameters

Both parameters must be set at installation time via a `SetHook` transaction.

| Parameter | Size | Description |
|-----------|------|-------------|
| `PCT` | 1 byte (`uint8`) | Percentage to forward (1–100). Example: `0x1E` = 30% |
| `DST` | 20 bytes | Destination account in raw 20-byte AccountID format |

## Behavior

1. **Filters** — Only `ttPAYMENT` transactions are processed; all others are accepted without action.
2. **Direction check** — Only incoming payments (where the hook account is the `Destination`) are processed; outgoing payments are skipped.
3. **Currency check** — Only native XAH (drops) are processed; IOU payments are skipped.
4. **Minimum threshold** — The calculated forward amount must be at least **10,000 drops (0.01 XAH)**. Payments below this threshold are accepted without forwarding.
5. **DST validation** — The destination account (`DST`) cannot be the same account as the hook account itself.
6. **Emit** — A new native XAH payment is emitted to `DST` for the calculated amount.

## Rollback Conditions

The hook rolls back (rejects the originating transaction) if:

- The `Amount` field cannot be read
- The `Amount` value is invalid
- The `PCT` parameter is missing or not exactly 1 byte
- The `DST` parameter is missing or not exactly 20 bytes
- `DST` is the same account as the hook account
- The emitted transaction fails

## Calculation

```
forward_drops = (received_drops * PCT) / 100
```

## Hook Hash

The hook is already deployed on both networks with the following hash:

```
3D3140B845F722C3832EDCF66CC7F60163292C90A7016B5FFE60B0D5D4DA537B
```

This hash is the same on **Mainnet** and **Testnet**.

## Installing

There are three ways to install this hook:

### Option 1 — Reference an already-deployed hook (by hash)

If the hook is already on-chain (see Hook Hash above), you can install it on your account without uploading the WASM again — just reference the hash in the `SetHook` transaction and omit `CreateCode`.

### Option 2 — Use the provided scripts

Two JavaScript helper scripts are included in this repository:

- **`create_hook.js`** — Deploys the WASM to the network and creates the hook on-chain.
- **`install_hook.js`** — Installs the already-deployed hook on a target account using the hook hash.

Install dependencies and run:

```sh
npm install
node create_hook.js   # deploy WASM and create hook
# or
node install_hook.js  # install hook by hash on your account
```

### Option 3 — Deploy manually with the compiled WASM

A pre-compiled `pct_split_payment.wasm` is included in this repository. Use it directly in a `SetHook` transaction with the required parameters:

```json
{
  "TransactionType": "SetHook",
  "Account": "<your-account>",
  "Hooks": [
    {
      "Hook": {
        "CreateCode": "<compiled-wasm-hex>",
        "HookOn": "0000000000000000",
        "HookNamespace": "<32-byte-hex-namespace>",
        "HookParameters": [
          {
            "HookParameter": {
              "HookParameterName": "504354",
              "HookParameterValue": "1E"
            }
          },
          {
            "HookParameter": {
              "HookParameterName": "445354",
              "HookParameterValue": "<20-byte-account-id-hex>"
            }
          }
        ]
      }
    }
  ]
}
```

> `504354` = `PCT` in hex, `445354` = `DST` in hex, `1E` = 30%.

