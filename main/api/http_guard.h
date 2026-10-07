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

/* Apply the standard HTTP guard plus same-origin validation for browser
 * requests that change device state. Non-browser LAN clients without Origin
 * remain supported. */
esp_err_t http_guard_handle_mutation(httpd_req_t *req, http_guard_handler_t next_handler);


/* OTA is never permitted while the open first-boot setup AP is active.
 * Normal LAN OTA then falls through to the mutation guard (and, when merged
 * with the physical-unlock branch, the stricter OTA authorization layer). */
esp_err_t http_guard_handle_ota_mutation(httpd_req_t *req, http_guard_handler_t next_handler);
