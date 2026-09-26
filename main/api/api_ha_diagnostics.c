/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#include "api/api_routes.h"

#include "cJSON.h"

#include "ha/ha_client.h"

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

esp_err_t api_ha_diagnostics_get_handler(httpd_req_t *req)
{
    ha_client_diagnostics_t diag;
    ha_client_get_diagnostics(&diag);

    cJSON *root = cJSON_CreateObject();
    cJSON *missing = cJSON_CreateArray();
    if (root == NULL || missing == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(missing);
        return httpd_resp_send_500(req);
    }

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "updated_unix_ms", (double)diag.updated_unix_ms);
    cJSON_AddNumberToObject(root, "missing_total", (double)diag.total);
    cJSON_AddNumberToObject(root, "missing_listed", (double)diag.listed);
    for (uint16_t i = 0; i < diag.listed; i++) {
        cJSON_AddItemToArray(missing, cJSON_CreateString(diag.names[i]));
    }
    cJSON_AddItemToObject(root, "missing_entities", missing);

    cJSON *connection_log = cJSON_CreateArray();
    if (connection_log == NULL) {
        cJSON_Delete(root);
        return httpd_resp_send_500(req);
    }
    for (uint16_t i = 0; i < diag.connection_log_count; i++) {
        cJSON *entry = cJSON_CreateObject();
        if (entry == NULL) continue;
        cJSON_AddNumberToObject(entry, "elapsed_ms", (double)diag.connection_log[i].elapsed_ms);
        cJSON_AddStringToObject(entry, "message", diag.connection_log[i].message);
        cJSON_AddItemToArray(connection_log, entry);
    }
    cJSON_AddItemToObject(root, "connection_log", connection_log);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}
