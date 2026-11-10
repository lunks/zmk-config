/*
 * Out-of-tree port of zmkfirmware/zmk#3182 (draft PR, not merged upstream):
 * slower split connection parameters while the keyboard is idle, normal ones when active.
 * Central only. Experimental: the peripherals need a Zephyr with the controller fix from
 * zmkfirmware/zephyr#51 (peripheral asserts when the central updates connection parameters).
 *
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

static void set_params(struct bt_conn *conn, void *data) {
    const struct bt_le_conn_param *param = data;
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    if (info.role == BT_CONN_ROLE_CENTRAL && info.state == BT_CONN_STATE_CONNECTED) {
        int err = bt_conn_le_param_update(conn, param);

        if (err) {
            LOG_DBG("Failed to update split connection params: %d", err);
        }
    }
}

static void apply(const struct bt_le_conn_param *param) {
    bt_conn_foreach(BT_CONN_TYPE_LE, set_params, (void *)param);
}

static int central_idle_listener(const zmk_event_t *eh) {
    static const struct bt_le_conn_param idle_param =
        BT_LE_CONN_PARAM_INIT(CONFIG_ZEN_SPLIT_BLE_IDLE_INT, CONFIG_ZEN_SPLIT_BLE_IDLE_INT,
                              CONFIG_ZEN_SPLIT_BLE_IDLE_LATENCY, CONFIG_ZEN_SPLIT_BLE_IDLE_TIMEOUT);
    static const struct bt_le_conn_param active_param =
        BT_LE_CONN_PARAM_INIT(CONFIG_ZMK_SPLIT_BLE_PREF_INT, CONFIG_ZMK_SPLIT_BLE_PREF_INT,
                              CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY, CONFIG_ZMK_SPLIT_BLE_PREF_TIMEOUT);

    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        LOG_DBG("Active: normal split connection parameters");
        apply(&active_param);
        break;
    case ZMK_ACTIVITY_IDLE:
        LOG_DBG("Idle: slower split connection parameters");
        apply(&idle_param);
        break;
    case ZMK_ACTIVITY_SLEEP:
        break;
    default:
        LOG_WRN("Unhandled activity state: %d", ev->state);
        return -EINVAL;
    }
    return 0;
}

ZMK_LISTENER(zen_central_idle, central_idle_listener);
ZMK_SUBSCRIPTION(zen_central_idle, zmk_activity_state_changed);
