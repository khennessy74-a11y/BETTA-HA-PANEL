/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 * Copyright (c) 2026 khennessy74-a11y
 */
#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

typedef esp_err_t (*http_guard_handler_t)(httpd_req_t *req);

esp_err_t http_guard_init(void);
esp_err_t http_guard_handle(httpd_req_t *req, http_guard_handler_t next_handler);

esp_err_t http_guard_handle_state_change(httpd_req_t *req, http_guard_handler_t next_handler);

/* Physically opening the on-device Admin controls grants a short-lived
 * maintenance window for protected Web UI mutations such as OTA. */
void http_guard_authorize_state_changes_for_ms(uint32_t duration_ms);
