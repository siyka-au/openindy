# UI verification checkpoint — widget specs

Three pieces of `.ui` layout for the UI verification checkpoint (see
`openindy-core/docs/domain-model.md` and the Claude Code plan file for full context). For each:
build the layout in Qt Creator to the object names below — arrangement, spacing, and styling are
entirely your call, only the object names matter, since that's what the wiring code and automated
tests key off. Ping me once a piece is built and I'll wire it up.

---

## 1. `CreateFeatureDialog` — scalar parameter entry

**File:** `openindy/ui/dialogs/createfeaturedialog.ui`

Add **one** plain, empty `QWidget` (no children, no layout assigned in Designer — I build both in
C++, same as the existing `widget_customParameter` in `featurefunctionsdialog.ui`), named:

- `widget_scalarParameters`

**Placement:** as a new row inside the existing `verticalLayout`, directly after the
`horizontalLayout_4` row (`label_nominalSystem` / `comboBox_nominalSystem`, ~line 178-196) and
before `verticalLayout_3` (the start/destination coordinate-system rows). This is where the
existing `initMeasurementConfigUI()` visibility-toggle logic already operates
(`createfeaturedialog.cpp:418-442`), so I can extend that same mechanism to show/hide this new
widget based on whether the currently-selected function has any declared parameters.

I fill it with a `ScalarParameterWidget` at runtime (same pattern `FeatureFunctionsDialog` already
uses for `widget_customParameter`) — you don't need to know anything about that widget's internals,
just leave this one placeholder empty.

---

## 2. `NominalPropertiesDialog` — rebuilt around the same mechanism

**File:** `openindy/ui/dialogs/nominalpropertiesdialog.ui`

**Remove:** the entire flat list of `label_nominal*` / `lineEdit_nominal*` row pairs (currently ~18
hardcoded pairs — x/y/z, i/j/k, r1/r2, angle, distance, temperature, aperture, length,
measurement series — lines ~41-320). This whole mechanism is the thing being replaced; none of
those object names are needed anymore.

**Add:** one plain, empty `QWidget`, same convention as above:

- `widget_scalarParameters`

**Placement:** where the removed field list was, inside the same `verticalLayout`, above the
existing OK/Cancel button row.

**Keep unchanged:** `toolButton_ok` / `toolButton_cancel` and their row (`horizontalLayout_10`,
~line 337+) — those stay exactly as they are; only what's above them gets replaced.

---

## 3. Filter pipeline panel

**File:** `openindy/ui/dialogs/featurefunctionsdialog.ui`

The target container already exists and is currently empty: `widget_filter`, with an assigned
`QGridLayout` named `gridLayout_3` (~line 314-338). Add real controls directly into it in Designer:

| Object name | Type | Notes |
|---|---|---|
| `checkBox_rejectOutliers` | `QCheckBox` | Label: "Reject outliers" |
| `spinBox_sigmaMultiplier` | `QDoubleSpinBox` | Suggested range 0.1–10.0, 1 decimal, default value 3.0. I'll wire it to only be enabled when the checkbox above is checked — no need to set that up in Designer. |
| `pushButton_applyFilter` | `QPushButton` | Label: "Apply" |
| `listWidget_exclusions` | `QListWidget` | Read-only display of current exclusions (element + reason), one per row. Set edit triggers to none if you want it visually obvious it's read-only, but I'll also enforce that in code either way. |

Arrangement within the grid is entirely your call — I only ever look these up by object name via
`findChild`, never by position.

---

## Not needed: owned sub-geometry

The "generate normal line / origin point" action is a context-menu item in `mainwindow.cpp`, not a
new dialog or panel — no `.ui` work needed for it. I can build and test that one independently, any
time.
