/**
 * pct_split_payment.c — Xahau Hook
 * ─────────────────────────────────────────────────────────────────────────────
 * Upon receiving an incoming native XAH payment, automatically forwards a
 * configurable percentage to a destination account.
 *
 * HookParameters (defined when installing with SetHook):
 *   "PCT"  → percentage to forward (1–100), 1 byte uint8
 *            Example: 0x1E = 30 (30%)
 *   "DST"  → destination account in 20-byte AccountID format (raw hex)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "hookapi.h"

// Minimum drops threshold to make it worth emitting (10000 drops = 0.01 XAH)
#define MIN_FORWARD_DROPS 10000LL

int64_t hook(uint32_t reserved)
{   
    _g(1,1);
    
    // ── 1. Filter: only ttPAYMENT ─────────────────────────────────────────────
    int64_t tt = otxn_type();
    if (tt != ttPAYMENT)
        accept(SBUF("split_payment: not a Payment, passing"), 0);

    // ── 2. Verify we are the destination (incoming payment) ───────────────────
    uint8_t hook_acc[20];
    uint8_t otxn_dst[20];

    hook_account(SBUF(hook_acc));

    if (otxn_field(SBUF(otxn_dst), sfDestination) != 20)
        accept(SBUF("split_payment: no destination field"), 0);

    // If we are NOT the destination → it is outgoing, skip
    if (!BUFFER_EQUAL_20(hook_acc, otxn_dst))
        accept(SBUF("split_payment: outgoing payment, skipping"), 0);

    // ── 3. Native XAH only (Amount as 8 bytes, not an IOU object) ────────────
    uint8_t amount_buf[48];
    int64_t amount_len = otxn_field(SBUF(amount_buf), sfAmount);
    if (amount_len < 0)
        rollback(SBUF("split_payment: could not read Amount"), 1);

    // If bit 0x80 of the first byte is set → it is an IOU
    if (amount_buf[0] & 0x80)
        accept(SBUF("split_payment: IOU payment, skipping"), 0);

    // Parse drops using AMOUNT_TO_DROPS
    int64_t drops = AMOUNT_TO_DROPS(amount_buf);
    if (drops < 0)
        rollback(SBUF("split_payment: invalid Amount"), 2);

    // ── 4. Read HookParameter "PCT" ───────────────────────────────────────────
    uint8_t pct_key[3] = {'P', 'C', 'T'};
    uint8_t pct_val[1];
    if (hook_param(SBUF(pct_val), SBUF(pct_key)) != 1)
        rollback(SBUF("split_payment: missing PCT parameter"), 3);

    uint8_t pct = pct_val[0];
    if (pct == 0 || pct > 100)
        accept(SBUF("split_payment: PCT is 0 or invalid, nothing to forward"), 0);

    // ── 5. Read HookParameter "DST" ───────────────────────────────────────────
    uint8_t dst_key[3] = {'D', 'S', 'T'};
    uint8_t dst_acc[20];
    if (hook_param(SBUF(dst_acc), SBUF(dst_key)) != 20)
        rollback(SBUF("split_payment: missing DST parameter"), 4);

    // DST cannot be the same account as the Hook
    if (BUFFER_EQUAL_20(hook_acc, dst_acc))
        rollback(SBUF("split_payment: DST cannot be hook account"), 5);

    // ── 6. Calculate drops to forward ─────────────────────────────────────────
    int64_t forward_drops = (drops * (int64_t)pct) / 100LL;

    if (forward_drops < MIN_FORWARD_DROPS)
    {
        accept(SBUF("split_payment: accepted without forwarding"), 0);
    }

    // ── 7. Prepare the emitted transaction using PREPARE_PAYMENT_SIMPLE macro ──
    etxn_reserve(1);

    uint8_t txn_buf[PREPARE_PAYMENT_SIMPLE_SIZE];
    PREPARE_PAYMENT_SIMPLE(txn_buf, forward_drops, dst_acc, 0, 0);

    // ── 8. Emit ───────────────────────────────────────────────────────────────
    uint8_t emit_result[32];
    if (emit(SBUF(emit_result), SBUF(txn_buf)) < 0)
        rollback(SBUF("split_payment: emit failed"), 7);

    accept(SBUF("split_payment: done"), 0);

    return 0;
}
