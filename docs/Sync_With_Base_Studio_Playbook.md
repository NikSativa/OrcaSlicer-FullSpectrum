# Sync With Base Studio — Playbook

**Trigger.** Run this playbook whenever the user asks to "sync with the base studio"
(RU: «синхронизировать с базовой студией») or otherwise asks to bring this fork up to
the official Snapmaker Orca base.

**Base studio.** Snapmaker Orca — `Snapmaker/OrcaSlicer` (the `snapmaker` git remote).
It is the OFFICIAL studio for the Snapmaker U1, so its feature set is the source of truth
for 100% U1 readiness. The sync target is its latest release tag; determine it with
`gh api repos/Snapmaker/OrcaSlicer/releases --jq '.[0].tag_name'` (as of the last sync
this was `v2.3.5`).

**Execution contract.** Execute Steps 1–4 end-to-end. Do NOT ask questions mid-run —
collect every open decision and report them only at the end, in Deliverable 2. Work on a
dedicated branch; never reset or overwrite `main`.

## Standing policy (decided 2026-07-22)

These resolved decisions OVERRIDE the generic steps below and narrow the sync to
"official base + custom filament presets + fork branding (official version numbers)":

- **Prefer official on any duplicate.** Wherever the official base already provides a
  feature the fork also implements (Mixed-Filament / FullSpectrum, the SSWCP/MQTT device
  stack, any bug already fixed upstream), take the official implementation. Report each
  such substitution afterward.
- **Carry over ONLY filament presets.** From the fork's profile data, re-apply only the
  custom *filament* presets (and their `filament_list` entries in `Snapmaker.json`).
  Machine and process presets stay 100% official — no ports, no deletions.
- **Do not restore in-house code features.** The fork's unique code (ObjColor import,
  FlushVolPredictor, InternalBridgeDetector, Sentry, spiral-lift warning, etc.) is dropped
  in favour of the official base. List what was dropped.
- **Branding = the fork's; version = official 1:1.** Restore the fork's icons and app
  naming (`SLIC3R_APP_NAME`/`SLIC3R_APP_KEY` = `SnapOrka`, the FullSpectrum identity, all
  icon/splash/about assets). Keep the version numbers identical to the official base tag
  (`Snapmaker_VERSION`, `SLIC3R_VERSION`) so future syncs know the baseline; do NOT
  reintroduce the fork's own version numbers or `FULLSPECTRUM_VERSION`.

---

## Step 1 — Copy the official base (100% U1-ready)

Take a pristine official Snapmaker Orca (latest release tag) as the base so that all
official U1 features work at 100%. Establish the three-way comparison used by every later
step:

- `MERGE_BASE = git merge-base HEAD snapmaker/main`
- `OFFICIAL   = <latest release tag>` (e.g. `v2.3.5`) — fetch with `git fetch snapmaker --tags`
- `HEAD       = ` current fork tip

Compute the **conflict zone** — files changed by BOTH sides since the merge base:

```bash
comm -12 \
  <(git diff --name-only "$MERGE_BASE" HEAD    | sort -u) \
  <(git diff --name-only "$MERGE_BASE" "$OFFICIAL" | sort -u)
```

Files only the fork changed re-apply cleanly; files in the conflict zone need a manual
three-way merge (see Guardrails).

## Step 2 — Carry over the user's custom filament presets

Re-apply every custom filament preset installed in the studio:

- In-repo presets the fork added that do not exist upstream, under
  `resources/profiles/Snapmaker/filament/` (e.g. third-party `Elegoo PETG U1`,
  `Creality TPU U1`, `Professional Lab TPU 95A U1`, `Sunlu PLA Silk U1`, the
  Polymaker/PolyLite/PolyTerra families, plus any Snapmaker-branded presets the user
  added or retuned).
- User-level presets in the runtime data dir
  (`~/Library/Application Support/SnapOrka/**/filament/` and any `Snapmaker_Orca` variant).

Split into: added (clean carry-over) vs. user-modified official presets (conflict risk if
the official base also changed them).

## Step 3 — Restore broken/reverted features on top

Produce a full list of features that the clean copy breaks or reverts (the fork's in-house
work) and re-apply all of them on top of the copied base. The in-house set includes, at
minimum:

- FullSpectrum / MixedFilament multi-material (`MixedFilament.*`, `FilamentColorLibrary.*`,
  the `mixed_filament_*` PrintConfig options, the Plater mixed-filament UI, the MMU gizmo
  count path, `ObjColorUtils`/`ObjColorDialog`).
- The `filamentsync/` module (device→slicer filament reconciliation).
- Device stack: vendored Paho MQTT C++ (`src/mqtt/`), `Utils/MQTT.*`, `Moonraker_Mqtt`
  (`Utils/MoonRaker.*`), Bonjour discovery, the SSWCP bridge.
- `FilamentHotBedNozzleRules`, the U1 bed-type system (`btGESP` / `enum_*_u1`), the
  WipeTower2 U1 branch, the spiral-lift boundary warning.
- The accumulated crash/startup/QoL bugfixes and the `test_mixed_filament.cpp` suite.

For each feature classify: clean re-apply (self-contained) vs. needs manual merge (a file
in the conflict zone).

## Step 4 — Port no-conflict features from other studios

Analyze the other studios (OrcaSlicer, Bambu Studio, ElegooSlicer, PrusaSlicer) and port
every feature that can be ported without conflict. Verify each candidate exists in a
RELEASED upstream tag before porting (past harvests mislabeled sources). Skip anything that
would collide with the conflict zone or the FullSpectrum stack; those go to Deliverable 2.

---

## Deliverables (report only at the very end)

1. **Features that will be LOST** from the new version because they could not be
   ported/re-applied — each with the reason.
2. **Features that conflict or need clarification** — the conflict-zone merges plus any
   decision only the user can make. This list is HEADED by the keep-vs-drop decision for
   the in-house FullSpectrum stack, because the official base does not contain it and a
   literal 100% copy would delete it.

---

## Guardrails

- Run on a dedicated branch (e.g. `sync/base-studio-<tag>`); never reset or force-overwrite
  `main`.
- The working-tree replacement plus three-way merges of conflict-zone files (last measured:
  346 files, of which ~168 are code, not profiles) is a real migration, not a mechanical
  copy. Execute the clean parts; leave conflict-zone merges and the FullSpectrum decision
  for Deliverable 2 rather than guessing.
- Released upstream tags are the source of truth. U1 process presets are frozen — never
  edit them; only filament profiles may be customized.
- Do not delete or overwrite the user's custom presets or in-house features as a side
  effect of "copying the official base"; they are preserved by Steps 2 and 3.
