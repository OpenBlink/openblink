/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file ble.c
 * @brief Implementation of Bluetooth Low Energy driver
 * @details Implements BLE initialization, connection management, and data
 * transfer functions
 */
#include "ble.h"

#include <assert.h>
#include <errno.h>
#include <soc.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "ble_blink.h"

LOG_MODULE_REGISTER(drv_ble, LOG_LEVEL_DBG);

/** @brief Global BLE context */
BLE_CONTEXT ble_context;

/** @brief Work item for restarting advertising from system workqueue */
static struct k_work adv_work;

/** @brief MTU exchange parameters */
static struct bt_gatt_exchange_params exchange_params;

/**
 * @brief Callback for MTU exchange completion
 *
 * @param conn Bluetooth connection handle
 * @param err Error code (0 for success)
 * @param params Exchange parameters
 */
static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
                            struct bt_gatt_exchange_params *params) {
  if (err) {
    LOG_ERR("BLE: Failed MTU exchange (err %u)", err);
    return;
  }
  uint16_t mtu = bt_gatt_get_mtu(conn);
  LOG_INF("BLE: Negotiated MTU: %u", mtu);
}

/**
 * @brief Connection callback
 *
 * @details Called when a BLE connection is established
 *
 * @param conn Bluetooth connection handle
 * @param err Error code (0 for success)
 */
static void on_connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("BLE: Connection failed (err 0x%02x %s)", err,
            bt_hci_err_to_str(err));
    return;
  }

  if (ble_context.conn) {
    bt_conn_unref(ble_context.conn);
  }
  ble_context.conn = bt_conn_ref(conn);

  // bt_conn_get_info() is for logging only; failure does not affect connection tracking
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG(
        "BLE: Connection established!\n\
        Connected to: %s\n\
        Role: %u\n\
        Connection interval: %u us\n\
        Slave latency: %u\n\
        Connection supervisory timeout: %u",
        addr, info.role, info.le.interval_us, info.le.latency, info.le.timeout);
  } else {
    LOG_ERR("BLE: Could not parse connection info");
  }

  exchange_params.func = mtu_exchange_cb;
  int ret = bt_gatt_exchange_mtu(conn, &exchange_params);
  if (ret) {
    LOG_ERR("BLE: Failed to start MTU exchange (err %d)", ret);
  }

  {
    BLE_PARAM param = {
        .event = BLE_EVENT_CONNECTED,
    };
    ble_context.event_cb(&param);
  }
}

/**
 * @brief Disconnection callback
 *
 * @details Called when a BLE connection is terminated
 *
 * @param conn Bluetooth connection handle
 * @param reason Reason for disconnection
 */
static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_INF("BLE: Disconnected (reason 0x%02x %s)", reason,
          bt_hci_err_to_str(reason));

  if (ble_context.conn == conn) {
    bt_conn_unref(ble_context.conn);
    ble_context.conn = NULL;
  }

  {
    BLE_PARAM param = {
        .event = BLE_EVENT_DISCONNECTED,
        .disconnected.reason = reason,
    };
    ble_context.event_cb(&param);
  }
}

/**
 * @brief Connection parameter request callback
 *
 * @details Called when a remote device requests connection parameter update
 *
 * @param conn Bluetooth connection handle
 * @param param Requested connection parameters
 * @return true if parameters are acceptable, false otherwise
 */
static bool on_le_param_req(struct bt_conn *conn,
                            struct bt_le_conn_param *param) {
  // Reject if supervision timeout is too short (< 100ms = 10 * 10ms units)
  if (param->timeout < 10) {
    LOG_WRN("BLE: Rejected conn param req: timeout %u too short",
            param->timeout);
    return false;
  }
  return true;
}

/**
 * @brief Connection parameter update callback
 *
 * @details Called when connection parameters have been updated
 *
 * @param conn Bluetooth connection handle
 * @param interval Connection interval
 * @param latency Slave latency
 * @param timeout Connection supervision timeout
 */
static void on_le_param_updated(struct bt_conn *conn, uint16_t interval,
                                uint16_t latency, uint16_t timeout) {
  struct bt_conn_info info;

  if (bt_conn_get_info(conn, &info)) {
    LOG_ERR("BLE: Could not parse connection info");
  } else {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG(
        "BLE: Connection parameters updated!\n\
        Connected to: %s\n\
        New Connection Interval: %u us\n\
        New Slave Latency: %u\n\
        New Connection Supervisory Timeout: %u",
        addr, info.le.interval_us, info.le.latency, info.le.timeout);
  }
}

/**
 * @brief Converts PHY value to string representation
 *
 * @param phy PHY value
 * @return const char* String representation of the PHY
 */
static const char *phy2str(uint8_t phy) {
  switch (phy) {
    case 0:
      return "No packets";
    case BT_GAP_LE_PHY_1M:
      return "LE 1M";
    case BT_GAP_LE_PHY_2M:
      return "LE 2M";
    case BT_GAP_LE_PHY_CODED:
      return "LE Coded";
    default:
      return "Unknown";
  }
}

/**
 * @brief PHY update callback
 *
 * @details Called when the PHY has been updated
 *
 * @param conn Bluetooth connection handle
 * @param param PHY information
 */
static void on_le_phy_updated(struct bt_conn *conn,
                              struct bt_conn_le_phy_info *param) {
  LOG_DBG("BLE: PHY updated: TX PHY %s, RX PHY %s", phy2str(param->tx_phy),
          phy2str(param->rx_phy));
}

/**
 * @brief Data length update callback
 *
 * @details Called when the data length parameters have been updated
 *
 * @param conn Bluetooth connection handle
 * @param info Data length information
 */
static void on_le_data_length_updated(struct bt_conn *conn,
                                      struct bt_conn_le_data_len_info *info) {
  LOG_DBG(
      "BLE: data len updated: TX (len: %d time: %d)"
      " RX (len: %d time: %d)",
      info->tx_max_len, info->tx_max_time, info->rx_max_len, info->rx_max_time);

  int mtu = bt_gatt_get_mtu(conn);
  LOG_DBG("BLE: MTU: %d", mtu);
}

/**
 * @brief Work handler for restarting advertising
 *
 * @details Called from the system workqueue to safely restart advertising
 * after a connection has been fully recycled
 *
 * @param work Work item pointer
 */
static void adv_work_handler(struct k_work *work) {
  ARG_UNUSED(work);
  int err = ble_start_advertising(bt_get_name());
  if (err) {
    LOG_ERR("BLE: Failed to restart advertising (err %d)", err);
  }
}

/**
 * @brief Connection recycled callback
 *
 * @details Called when the connection object has been fully recycled
 * and is available for reuse. This is the safe point to restart advertising.
 */
static void on_recycled(void) {
  LOG_DBG("BLE: Connection object recycled, restarting advertising");
  int ret = k_work_submit(&adv_work);
  if (ret && ret != -EALREADY) {
    LOG_WRN("BLE: Failed to submit advertising work (err %d)", ret);
  }
}

/**
 * @brief Security level changed callback
 *
 * @details Called when the security level of a connection has changed.
 * Enabled when CONFIG_BT_SMP is active.
 *
 * @param conn Bluetooth connection handle
 * @param level New security level
 * @param err Security error code (0 for success)
 */
static void on_security_changed(struct bt_conn *conn, bt_security_t level,
                                enum bt_security_err err) {
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (!err) {
    LOG_INF("BLE: Security changed: %s level %u", addr, level);
  } else {
    LOG_WRN("BLE: Security failed: %s level %u err %d %s", addr, level, err,
            bt_security_err_to_str(err));
  }
}

// Connection callbacks registered at file scope (compile-time initialization)
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = on_recycled,
    .security_changed = on_security_changed,
    .le_param_req = on_le_param_req,
    .le_param_updated = on_le_param_updated,
    .le_phy_updated = on_le_phy_updated,
    .le_data_len_updated = on_le_data_length_updated,
};

/**
 * @brief Initializes the BLE subsystem
 *
 * @details Sets up the BLE stack, registers callbacks, and initializes the
 * Blink service
 *
 * @param cb Callback function for BLE events
 * @return int 0 on success, negative on error
 */
int ble_init(BLE_CALLBACK cb) {
  int err = 0;
  assert(cb != NULL);

  memset(&ble_context, 0x00, sizeof(ble_context));

  // Save callback
  ble_context.event_cb = cb;

  // Initialize advertising work item
  k_work_init(&adv_work, adv_work_handler);

  // Enable Bluetooth (synchronous: blocks until BLE stack is ready)
  LOG_DBG("BLE: bt_enable()");
  err = bt_enable(NULL);
  if (err) {
    LOG_ERR("BLE: initialization failed (err %d)", err);
    return err;
  }

  // Load settings after BLE stack is ready
  LOG_DBG("BLE: settings_load()");
  int settings_err = settings_load();
  if (settings_err) {
    LOG_WRN("BLE: settings_load() returned err %d", settings_err);
  }

  // Clear any stale bonding information from flash.
  // Currently CONFIG_BT_BONDABLE=n, so no new bonds are created at runtime.
  // However, residual bond data may exist from previous firmware versions or
  // configuration changes. Clearing it on every boot ensures a clean state
  // and avoids occupying the limited pairing slots (CONFIG_BT_MAX_PAIRED=1).
  // When bonding is enabled in the future, this call should be removed or
  // replaced with selective cleanup logic.
  int unpair_err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
  if (unpair_err && unpair_err != -ENOENT) {
    LOG_WRN("BLE: Failed to clear bonding info (err %d)", unpair_err);
  }

  // Register GATT service after BLE stack is ready
  err = ble_blink_init();
  if (err) {
    LOG_ERR("BLE: Blink service registration failed (err %d)", err);
    return err;
  }

  {
    BLE_PARAM param = {
        .event = BLE_EVENT_INITIALIZED,
    };
    ble_context.event_cb(&param);
  }

  return err;
}

/**
 * @brief Disconnects the current BLE connection
 *
 * @return int 0 on success, negative on error
 */
int ble_disconnect() {
  LOG_INF("BLE: Disconnecting...");
  struct bt_conn *conn = ble_context.conn;
  if (conn == NULL) {
    return 0;
  }

  // Take our own reference to ensure the connection object remains valid
  // even if on_disconnected fires concurrently and releases ble_context.conn.
  conn = bt_conn_ref(conn);
  int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
  if (err) {
    LOG_ERR("BLE: Failed to disconnect (err %d)", err);
  }
  bt_conn_unref(conn);
  return err;
}

/**
 * @brief Scan response data
 * @details Includes the OpenBlink service UUID
 */
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, OPENBLINK_SERVICE_UUID),
};

/**
 * @brief Starts BLE advertising with the specified device name
 *
 * @param local_name Device name to advertise
 * @return int 0 on success, negative on error
 */
int ble_start_advertising(const char *local_name) {
  int err = 0;

  // Advertisement packet
  // BT_LE_AD_GENERAL: 	General Discoverable.
  // BT_LE_AD_NO_BREDR: BR/EDR not supported.
  struct bt_data ad[] = {
      BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
      BT_DATA(BT_DATA_NAME_COMPLETE, local_name, strlen(local_name)),
  };

  //	bt_set_name (const char *name)
  // 	bt_set_appearance (uint16_t new_appearance)

  // Start advertising
  err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd,
                        ARRAY_SIZE(sd));
  if (err) {
    LOG_ERR("BLE: Advertising failed to start (err %d)", err);
    ble_context.advertising = false;
  } else {
    ble_context.advertising = true;
  }

  return err;
}

/**
 * @brief Stops BLE advertising
 *
 * @return int 0 on success, negative on error
 */
int ble_stop_advertising() {
  int err = bt_le_adv_stop();
  if (err) {
    LOG_ERR("BLE: Failed to stop advertising (err %d)", err);
  } else {
    ble_context.advertising = false;
  }
  return err;
}

/**
 * @brief Gets the current Maximum Transmission Unit (MTU)
 *
 * @return uint16_t Current MTU size in bytes
 */
uint16_t ble_get_mtu() {
  struct bt_conn *conn = ble_context.conn;
  if (conn == NULL) {
    return BLE_ATT_MTU_DEFAULT;
  }

  // Take our own reference to ensure the connection object remains valid
  conn = bt_conn_ref(conn);
  uint16_t mtu = bt_gatt_get_mtu(conn);
  bt_conn_unref(conn);
  return mtu;
}

/**
 * @brief Gets the current BLE state
 *
 * @details Derives state from driver-managed state:
 *          - Connected: ble_context.conn != NULL (bt_conn_ref managed)
 *          - Advertising: ble_context.advertising && not connected
 *          - Off: BLE stack not initialized
 *
 * @return int 0=Off, 1=Advertising, 2=Connected
 */
int ble_get_state(void) {
  if (ble_context.conn != NULL) {
    return 2;
  }
  if (ble_context.advertising) {
    return 1;
  }
  return 0;
}
