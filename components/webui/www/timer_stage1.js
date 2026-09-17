/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 *
 * BETTA HA Panel
 * Web UI Stage 1 - Timer editor
 *
 * This file intentionally extends the existing Web UI without replacing
 * app.js. It adds timer support to the existing layout editor and keeps
 * the normal /api/layout save/load path.
 */

(() => {
  "use strict";

  const TIMER_TYPE = "timer";

  const DEFAULT_TIMER_TITLE = "Timer";
  const DEFAULT_TIMER_WIDTH = 300;
  const DEFAULT_TIMER_HEIGHT = 200;

  const DEFAULT_TIMER_SHOW_TITLE = true;
  const DEFAULT_TIMER_SHOW_ICON = true;
  const DEFAULT_TIMER_SHOW_STATE = true;

  const DEFAULT_TIMER_SHOW_START = true;
  const DEFAULT_TIMER_SHOW_PAUSE = true;
  const DEFAULT_TIMER_SHOW_CANCEL = true;
  const DEFAULT_TIMER_SHOW_FINISH = true;

  const TIMER_ICON_MODES = new Set([
    "automatic",
    "custom",
  ]);

  function getSelectedWidgetSafe() {
    try {
      if (typeof selectedWidget === "function") {
        return selectedWidget();
      }
    } catch (_) {
      // Ignore and fall back to the editor model.
    }

    try {
      if (
        typeof editor !== "undefined" &&
        editor &&
        Array.isArray(editor.layout?.pages)
      ) {
        for (const page of editor.layout.pages) {
          if (!Array.isArray(page.widgets)) continue;

          const widget = page.widgets.find(
            (item) => item && item.id === editor.selectedWidgetId
          );

          if (widget) return widget;
        }
      }
    } catch (_) {
      // Ignore.
    }

    return null;
  }

  function getSelectedPageSafe() {
    try {
      if (typeof selectedPage === "function") {
        return selectedPage();
      }
    } catch (_) {
      // Ignore.
    }

    try {
      if (
        typeof editor !== "undefined" &&
        editor &&
        Array.isArray(editor.layout?.pages)
      ) {
        return editor.layout.pages.find(
          (page) => page && page.id === editor.selectedPageId
        ) || null;
      }
    } catch (_) {
      // Ignore.
    }

    return null;
  }

  function refreshExistingEditor() {
    try {
      if (typeof renderInspector === "function") {
        renderInspector();
      }
    } catch (_) {
      // Ignore.
    }

    try {
      if (typeof renderWidgets === "function") {
        renderWidgets();
      }
    } catch (_) {
      // Ignore.
    }

    try {
      if (typeof renderCanvas === "function") {
        renderCanvas();
      }
    } catch (_) {
      // Ignore.
    }
  }

  function refreshAllEditor() {
    try {
      if (typeof renderAll === "function") {
        renderAll();
        return;
      }
    } catch (_) {
      // Fall back to the individual render functions.
    }

    refreshExistingEditor();
  }

  function makeWidgetId(page) {
    const existing = new Set(
      Array.isArray(page?.widgets)
        ? page.widgets.map((widget) => String(widget?.id || ""))
        : []
    );

    let index = 1;

    while (existing.has(`timer_${index}`)) {
      index++;
    }

    return `timer_${index}`;
  }

  function findFirstTimerEntity() {
    try {
      if (
        typeof editor !== "undefined" &&
        editor &&
        editor.states &&
        typeof editor.states.keys === "function"
      ) {
        for (const entityId of editor.states.keys()) {
          if (String(entityId).startsWith("timer.")) {
            return String(entityId);
          }
        }
      }
    } catch (_) {
      // Ignore.
    }

    return "timer.example";
  }

function refreshTimerEntityOptions() {
  const list =
    document.getElementById("timerEntityOptions");

  if (!list) {
    return;
  }

  const entityIds = [];

  try {
    if (
      typeof editor !== "undefined" &&
      editor &&
      editor.states &&
      typeof editor.states.keys === "function"
    ) {
      for (const entityId of editor.states.keys()) {
        const value = String(entityId || "");

        if (value.startsWith("timer.")) {
          entityIds.push(value);
        }
      }
    }
  } catch (_) {
    // Leave the list empty if HA states are unavailable.
  }

  entityIds.sort((a, b) => a.localeCompare(b));

  list.replaceChildren();

  for (const entityId of entityIds) {
    const option = document.createElement("option");
    option.value = entityId;
    list.appendChild(option);
  }
}
  
  function addTimerWidget() {
    const page = getSelectedPageSafe();

    if (!page) {
      return;
    }

    if (
      typeof isEnergyPage === "function" &&
      isEnergyPage(page)
    ) {
      try {
        if (typeof setStatus === "function") {
          setStatus(
            "Energy pages do not accept widgets.",
            true
          );
        }
      } catch (_) {
        // Ignore.
      }

      return;
    }

    if (!Array.isArray(page.widgets)) {
      page.widgets = [];
    }

    const id = makeWidgetId(page);

    let canvasWidth = 720;
    let canvasHeight = 600;

    try {
      if (typeof CANVAS_WIDTH !== "undefined") {
        canvasWidth = Number(CANVAS_WIDTH) || canvasWidth;
      }

      if (typeof CANVAS_HEIGHT !== "undefined") {
        canvasHeight = Number(CANVAS_HEIGHT) || canvasHeight;
      }
    } catch (_) {
      // Keep defaults.
    }

    const width = Math.min(
      DEFAULT_TIMER_WIDTH,
      Math.max(180, canvasWidth)
    );

    const height = Math.min(
      DEFAULT_TIMER_HEIGHT,
      Math.max(130, canvasHeight)
    );

    const entityId = findFirstTimerEntity();

    const widget = {
      id,
      type: TIMER_TYPE,
      title: DEFAULT_TIMER_TITLE,
      entity_id: entityId,

      /*
       * Empty icon means automatic icon selection.
       */
      icon: "",

      show_title: DEFAULT_TIMER_SHOW_TITLE,
      show_icon: DEFAULT_TIMER_SHOW_ICON,
      show_state: DEFAULT_TIMER_SHOW_STATE,

      timer_show_start: DEFAULT_TIMER_SHOW_START,
      timer_show_pause: DEFAULT_TIMER_SHOW_PAUSE,
      timer_show_cancel: DEFAULT_TIMER_SHOW_CANCEL,
      timer_show_finish: DEFAULT_TIMER_SHOW_FINISH,

      rect: {
        x: 20,
        y: 20,
        w: width,
        h: height,
      },
    };

    page.widgets.push(widget);

    try {
      editor.selectedWidgetId = id;
    } catch (_) {
      // Ignore.
    }

    refreshAllEditor();

    /*
     * Re-sync our timer controls after the normal editor render.
     */
    window.setTimeout(() => {
      syncTimerInspector(true);
    }, 0);
  }

function bindTimerButton() {
  const button =
    document.getElementById("addTimerBtn");

  if (!button) {
    return;
  }

  /*
   * index.html now owns the Timer button.
   * Only attach the Timer widget behaviour here.
   */
  if (button.dataset.timerStage1Bound === "1") {
    return;
  }

  button.dataset.timerStage1Bound = "1";

  button.addEventListener("click", (event) => {
    event.preventDefault();
    addTimerWidget();
  });
}

  function ensureTimerTypeOption() {
    const typeSelect = document.getElementById("fType");

    if (!typeSelect) {
      return;
    }

    if (
      !Array.from(typeSelect.options).some(
        (option) => option.value === TIMER_TYPE
      )
    ) {
      const option = document.createElement("option");

      option.value = TIMER_TYPE;
      option.textContent = TIMER_TYPE;

      typeSelect.appendChild(option);
    }
  }

  function ensureTimerInspector() {
    if (document.getElementById("timerOptions")) {
      return document.getElementById("timerOptions");
    }

    const inspectorBody =
      document.getElementById("inspectorSectionBody");

    if (!inspectorBody) {
      return null;
    }

    const styleGroup =
      inspectorBody.querySelector(".inspector-group");

    const container = document.createElement("div");

    container.id = "timerOptions";
    container.className = "timer-options hidden";

    container.innerHTML = `
      <div class="timer-stage1-heading">
        <strong>Timer</strong>
      </div>

      <label>
        <span>Show title</span>
        <input
          id="fTimerShowTitle"
          type="checkbox"
          checked
        />
      </label>

      <label>
        <span>Show icon</span>
        <input
          id="fTimerShowIcon"
          type="checkbox"
          checked
        />
      </label>

      <label>
        <span>Show state</span>
        <input
          id="fTimerShowState"
          type="checkbox"
          checked
        />
      </label>

      <label>
        <span>Icon</span>
        <select id="fTimerIconMode">
          <option value="automatic">Automatic</option>
          <option value="custom">Custom MDI icon</option>
        </select>
      </label>

      <label id="fTimerCustomIconWrap" class="hidden">
        <span>Custom MDI icon</span>
        <input
          id="fTimerCustomIcon"
          type="text"
          maxlength="63"
          placeholder="mdi:timer-outline"
          autocomplete="off"
          spellcheck="false"
        />
      </label>

      <div class="timer-stage1-controls">
        <div class="timer-stage1-heading">
          <strong>Timer controls</strong>
        </div>

        <label>
          <span>Show Start</span>
          <input
            id="fTimerShowStart"
            type="checkbox"
            checked
          />
        </label>

        <label>
          <span>Show Pause</span>
          <input
            id="fTimerShowPause"
            type="checkbox"
            checked
          />
        </label>

        <label>
          <span>Show Cancel</span>
          <input
            id="fTimerShowCancel"
            type="checkbox"
            checked
          />
        </label>

        <label>
          <span>Show Finish</span>
          <input
            id="fTimerShowFinish"
            type="checkbox"
            checked
          />
        </label>
      </div>
    `;

    if (styleGroup) {
      const styleBody =
        styleGroup.querySelector(".inspector-group-body");

      if (styleBody) {
        styleBody.appendChild(container);
      } else {
        styleGroup.appendChild(container);
      }
    } else {
      inspectorBody.prepend(container);
    }

    const customIconWrap =
      document.getElementById("fTimerCustomIconWrap");

    const iconMode =
      document.getElementById("fTimerIconMode");

    if (iconMode) {
      iconMode.addEventListener("change", () => {
        const widget = getSelectedWidgetSafe();

        if (!widget || widget.type !== TIMER_TYPE) {
          return;
        }

        updateTimerIconMode(widget);
      });
    }

    if (customIconWrap) {
      customIconWrap.classList.add("hidden");
    }

    bindTimerInspectorEvents();

    return container;
  }

  function bindTimerInspectorEvents() {
    const checkboxMap = [
      [
        "fTimerShowTitle",
        "show_title",
        DEFAULT_TIMER_SHOW_TITLE,
      ],
      [
        "fTimerShowIcon",
        "show_icon",
        DEFAULT_TIMER_SHOW_ICON,
      ],
      [
        "fTimerShowState",
        "show_state",
        DEFAULT_TIMER_SHOW_STATE,
      ],
      [
        "fTimerShowStart",
        "timer_show_start",
        DEFAULT_TIMER_SHOW_START,
      ],
      [
        "fTimerShowPause",
        "timer_show_pause",
        DEFAULT_TIMER_SHOW_PAUSE,
      ],
      [
        "fTimerShowCancel",
        "timer_show_cancel",
        DEFAULT_TIMER_SHOW_CANCEL,
      ],
      [
        "fTimerShowFinish",
        "timer_show_finish",
        DEFAULT_TIMER_SHOW_FINISH,
      ],
    ];

    for (const [
      elementId,
      property,
      defaultValue,
    ] of checkboxMap) {
      const element = document.getElementById(elementId);

      if (!element || element.dataset.timerStage1Bound === "1") {
        continue;
      }

      element.dataset.timerStage1Bound = "1";

      element.addEventListener("change", () => {
        const widget = getSelectedWidgetSafe();

        if (!widget || widget.type !== TIMER_TYPE) {
          return;
        }

        widget[property] =
          Boolean(element.checked);

        refreshExistingEditor();
      });
    }

    const customIcon =
      document.getElementById("fTimerCustomIcon");

    if (customIcon && customIcon.dataset.timerStage1Bound !== "1") {
      customIcon.dataset.timerStage1Bound = "1";

      customIcon.addEventListener("input", () => {
        const widget = getSelectedWidgetSafe();

        if (!widget || widget.type !== TIMER_TYPE) {
          return;
        }

        const value =
          String(customIcon.value || "").trim();

        widget.icon = value;

        refreshExistingEditor();
      });
    }
  }

  function updateTimerIconMode(widget) {
    const mode =
      document.getElementById("fTimerIconMode");

    const customIcon =
      document.getElementById("fTimerCustomIcon");

    if (!mode || !customIcon) {
      return;
    }

    const resolvedMode =
      TIMER_ICON_MODES.has(mode.value)
        ? mode.value
        : "automatic";

    const customWrap =
      document.getElementById("fTimerCustomIconWrap");

    if (customWrap) {
      customWrap.classList.toggle(
        "hidden",
        resolvedMode !== "custom"
      );
    }

    if (resolvedMode === "automatic") {
      widget.icon = "";
      customIcon.value = "";
    } else {
      widget.icon =
        String(customIcon.value || "").trim();
    }
  }

  function syncTimerInspector(force = false) {
  const widget = getSelectedWidgetSafe();

  const options =
    ensureTimerInspector();

  if (!options) {
    return;
  }

  const isTimer =
    Boolean(widget && widget.type === TIMER_TYPE);

  options.classList.toggle(
    "hidden",
    !isTimer
  );

  /*
   * Restore the normal entity list whenever the selected
   * widget is not a Timer.
   */
  if (!isTimer) {
    const entity =
      document.getElementById("fEntity");

    if (entity) {
      entity.setAttribute(
        "list",
        "entityOptions"
      );
    }

    return;
  }

  const title =
    document.getElementById("fTitle");

  const entity =
    document.getElementById("fEntity");

  const type =
    document.getElementById("fType");

  /*
   * Keep the normal title/entity controls synchronised.
   *
   * The timer event interception below prevents the existing widget
   * validator from rejecting timer.* entities.
   */
  if (title && (force || document.activeElement !== title)) {
    title.value =
      String(widget.title || "");
  }

  /*
   * Timer widgets use a dedicated entity suggestion list
   * containing only timer.* Home Assistant entities.
   */
  if (entity) {
    entity.setAttribute(
      "list",
      "timerEntityOptions"
    );

    refreshTimerEntityOptions();
  }

  if (entity && (force || document.activeElement !== entity)) {
    entity.value =
      String(widget.entity_id || "");
  }

  if (type) {
    type.value = TIMER_TYPE;
  }

  const defaults = {
    show_title: DEFAULT_TIMER_SHOW_TITLE,
    show_icon: DEFAULT_TIMER_SHOW_ICON,
    show_state: DEFAULT_TIMER_SHOW_STATE,
    timer_show_start: DEFAULT_TIMER_SHOW_START,
    timer_show_pause: DEFAULT_TIMER_SHOW_PAUSE,
    timer_show_cancel: DEFAULT_TIMER_SHOW_CANCEL,
    timer_show_finish: DEFAULT_TIMER_SHOW_FINISH,
  };

  for (const [property, defaultValue] of Object.entries(defaults)) {
    if (typeof widget[property] !== "boolean") {
      widget[property] = defaultValue;
    }
  }

  if (typeof widget.icon !== "string") {
    widget.icon = "";
  }

  const controls = [
    ["fTimerShowTitle", widget.show_title],
    ["fTimerShowIcon", widget.show_icon],
    ["fTimerShowState", widget.show_state],
    ["fTimerShowStart", widget.timer_show_start],
    ["fTimerShowPause", widget.timer_show_pause],
    ["fTimerShowCancel", widget.timer_show_cancel],
    ["fTimerShowFinish", widget.timer_show_finish],
  ];

  for (const [id, value] of controls) {
    const control = document.getElementById(id);

    if (
      control &&
      (force || document.activeElement !== control)
    ) {
      control.checked = Boolean(value);
    }
  }

  const iconMode =
    document.getElementById("fTimerIconMode");

  const customIcon =
    document.getElementById("fTimerCustomIcon");

  const customWrap =
    document.getElementById("fTimerCustomIconWrap");

  const customIconValue =
    String(widget.icon || "").trim();

  const resolvedIconMode =
    customIconValue
      ? "custom"
      : "automatic";

  if (
    iconMode &&
    (force || document.activeElement !== iconMode)
  ) {
    iconMode.value = resolvedIconMode;
  }

  if (
    customIcon &&
    (force || document.activeElement !== customIcon)
  ) {
    customIcon.value = customIconValue;
  }

  if (customWrap) {
    customWrap.classList.toggle(
      "hidden",
      resolvedIconMode !== "custom"
    );
  }
}

  function interceptTimerCoreInspectorEvents() {
    /*
     * The existing editor validates entities according to known widget
     * types. Timer support was added after that validator, so intercept
     * title/entity edits for timer widgets and update the layout model
     * directly.
     *
     * Capture phase is intentional: it prevents the older validator from
     * rejecting timer.* before our handler runs.
     */
    document.addEventListener(
      "input",
      (event) => {
        const target = event.target;

        if (
          !target ||
          (target.id !== "fTitle" &&
            target.id !== "fEntity")
        ) {
          return;
        }

        const widget = getSelectedWidgetSafe();

        if (!widget || widget.type !== TIMER_TYPE) {
          return;
        }

        event.stopImmediatePropagation();

        if (target.id === "fTitle") {
          widget.title =
            String(target.value || "").trim();
        } else if (target.id === "fEntity") {
          const value =
            String(target.value || "").trim();

          /*
           * Do not prevent editing. The user can type an entity even when
           * HA has not supplied its state yet.
           */
          widget.entity_id = value;
        }

        refreshExistingEditor();

        /*
         * Restore the user's input after the canvas render.
         */
        window.setTimeout(() => {
          if (
            target &&
            document.contains(target)
          ) {
            target.value =
              target.id === "fTitle"
                ? String(widget.title || "")
                : String(widget.entity_id || "");
          }
        }, 0);
      },
      true
    );

    document.addEventListener(
      "change",
      (event) => {
        const target = event.target;

        if (
          !target ||
          (target.id !== "fTitle" &&
            target.id !== "fEntity")
        ) {
          return;
        }

        const widget = getSelectedWidgetSafe();

        if (!widget || widget.type !== TIMER_TYPE) {
          return;
        }

        event.stopImmediatePropagation();

        if (target.id === "fTitle") {
          widget.title =
            String(target.value || "").trim();
        } else {
          widget.entity_id =
            String(target.value || "").trim();
        }

        refreshExistingEditor();
      },
      true
    );
  }

  function addTimerStyles() {
    if (document.getElementById("timerStage1Styles")) {
      return;
    }

    const style = document.createElement("style");

    style.id = "timerStage1Styles";

    style.textContent = `
      #timerOptions {
        margin-top: 12px;
        padding: 12px;
        border: 1px solid rgba(127, 160, 190, 0.28);
        border-radius: 10px;
        background: rgba(0, 0, 0, 0.08);
      }

      #timerOptions.hidden {
        display: none !important;
      }

      #timerOptions label {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        margin: 8px 0;
      }

      #timerOptions label > span {
        flex: 1;
      }

      #timerOptions input[type="checkbox"] {
        flex: 0 0 auto;
        width: auto;
      }

      #timerOptions input[type="text"],
      #timerOptions select {
        min-width: 150px;
      }

      .timer-stage1-heading {
        margin: 2px 0 8px;
        font-size: 0.9rem;
        opacity: 0.85;
      }

      .timer-stage1-controls {
        margin-top: 14px;
        padding-top: 10px;
        border-top: 1px solid rgba(127, 160, 190, 0.2);
      }
    `;

    document.head.appendChild(style);
  }

  function initialiseTimerStage1() {
    addTimerStyles();
    ensureTimerTypeOption();
    bindTimerButton();
    ensureTimerInspector();
    interceptTimerCoreInspectorEvents();

    /*
     * The existing editor completes its own initial render first.
     * The polling loop then keeps our timer inspector synchronised when
     * selection changes.
     */
    let lastWidgetId = null;

    window.setInterval(() => {
      try {
        const widget = getSelectedWidgetSafe();
        const widgetId = widget?.id || null;

        const changed =
          widgetId !== lastWidgetId;

        lastWidgetId = widgetId;

        syncTimerInspector(changed);
      } catch (_) {
        // Never allow Stage 1 to break the main editor.
      }
    }, 250);

    /*
     * If the editor is already rendered when this script is loaded,
     * perform one immediate sync.
     */
    window.setTimeout(() => {
      syncTimerInspector(true);
    }, 0);
  }

  if (document.readyState === "loading") {
    document.addEventListener(
      "DOMContentLoaded",
      initialiseTimerStage1,
      { once: true }
    );
  } else {
    initialiseTimerStage1();
  }
})();
