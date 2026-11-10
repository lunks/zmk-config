if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_TRI_STATE app PRIVATE behaviors/behavior_tri_state.c)
endif()

target_sources_ifdef(CONFIG_ZEN_SPLIT_AUTO_UNPAIR_ON_KEY_MISMATCH app PRIVATE split/auto_unpair.c)
target_sources_ifdef(CONFIG_ZEN_SPLIT_BLE_IDLE_PARAMS app PRIVATE split/central_idle.c)
