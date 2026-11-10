/*
 * Out-of-tree port of zmkfirmware/zmk#3377 (open PR, not merged upstream):
 * clear a stale split bond when a split peripheral reports a missing key.
 *
 * Differences from the PR:
 *  - registers its own bt_conn_cb instead of patching app/src/ble.c, so no ZMK fork is needed
 *  - only acts on links where this device is the BLE central (the split peripheral links);
 *    host (HID) links are peripheral-role and are never touched, which the PR's #ifdef alone does not guarantee
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void auto_unpair_security_changed(struct bt_conn *conn, bt_security_t level,
                                         enum bt_security_err err) {
    if (err != BT_SECURITY_ERR_PIN_OR_KEY_MISSING) {
        return;
    }

    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) < 0 || info.role != BT_CONN_ROLE_CENTRAL) {
        return;
    }

    LOG_WRN("Stale split bond detected (peripheral lost its key), clearing and disconnecting");

    int ret = bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
    if (ret < 0) {
        LOG_ERR("Failed to clear the split bond (%d)", ret);
    }

    ret = bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
    if (ret < 0) {
        LOG_ERR("Failed to disconnect the split peripheral (%d)", ret);
    }
}

BT_CONN_CB_DEFINE(zen_auto_unpair_cb) = {
    .security_changed = auto_unpair_security_changed,
};
