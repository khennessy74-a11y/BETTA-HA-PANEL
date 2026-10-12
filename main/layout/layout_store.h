/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 * Copyright (c) 2026 khennessy74-a11y
 */
#pragma once

#include <stddef.h>
#include "esp_err.h"

esp_err_t layout_store_init(void);
esp_err_t layout_store_load(char **json_out);
esp_err_t layout_store_save(const char *json);
esp_err_t layout_store_set_active_profile(const char *profile);
esp_err_t layout_store_get_active_profile(char *profile_out, size_t profile_out_size);
const char *layout_store_default_json(void);
