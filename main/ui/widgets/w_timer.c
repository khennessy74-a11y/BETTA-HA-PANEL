typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];

    lv_obj_t *card;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *state;
    lv_obj_t *remaining;

    lv_obj_t *start_button;
    lv_obj_t *pause_button;
    lv_obj_t *cancel_button;
    lv_obj_t *finish_button;

    bool show_icon;
    bool show_state;

    bool show_start;
    bool show_pause;
    bool show_cancel;
    bool show_finish;

    int64_t finishes_at_ms;
    int64_t remaining_ms;

    bool unavailable;
} w_timer_ctx_t;

if (strcmp(state->state, "active") == 0) {
    /* Use finishes_at when available. */
}

if (strcmp(state->state, "paused") == 0) {
    /* Display remaining, but don't decrement. */
}

if (strcmp(state->state, "idle") == 0) {
    /* Display 00:00. */
}

cJSON *root = cJSON_Parse(state->attributes_json);
if (root != NULL) {
    cJSON *remaining =
        cJSON_GetObjectItemCaseSensitive(root, "remaining");

    cJSON *finishes_at =
        cJSON_GetObjectItemCaseSensitive(root, "finishes_at");

    ...
    
    cJSON_Delete(root);
}
lv_timer_create(timer_tick_cb, 1000, ctx);
