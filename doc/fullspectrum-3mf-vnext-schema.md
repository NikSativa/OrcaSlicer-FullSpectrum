# FullSpectrum 3MF vNext Schema Proposal

## Status

Draft proposal for a next-generation FullSpectrum `.3mf` package profile.

This is not a description of the current implementation. It is a proposed replacement architecture intended to be:

- extendable
- backwards compatible
- forwards compatible
- suitable for publication as a vendor-neutral profile on top of 3MF / OPC

## Why A New Schema

The current package works, but its persistence format has some weak spots:

- too much meaning is packed into ad hoc files and positional XML/CSV-like payloads
- `mixed_filament_definitions` is compact but not self-describing
- there is no dedicated per-part schema versioning model
- stable logical identity is mixed together with transient export order
- project intent, derived slice data, and machine-specific outputs are not cleanly separated

The goal of this proposal is to define one canonical source of truth for FullSpectrum project data while still allowing:

- old consumers to keep reading a legacy projection
- new consumers to ignore optional future fields safely
- editors to preserve unknown extension parts when round-tripping a package

## External Design Constraints

This proposal is deliberately aligned with official 3MF rules:

- 3MF documents are OPC/ZIP packages with parts and relationships.
- 3MF supports extensions through namespaces and custom parts.
- custom OPC parts can be attached to the package root and marked with `MustPreserve`.
- 3MF already defines package-level digital signatures, so trust-sensitive workflows should reuse that mechanism.
- consumers are expected to ignore unknown namespaces they do not support.

Official references:

- 3MF specification index: https://3mf.io/spec/
- 3MF Core Specification: https://3mf.io/spec/core-v1-3-0/
- GitHub copy of the 3MF Core Specification: https://github.com/3MFConsortium/spec_core/blob/master/3MF%20Core%20Specification.md

## Design Principles

### 1. One Canonical Source Of Truth

There must be a single canonical representation for FullSpectrum project semantics.

Legacy projections may be written for interoperability, but they are derived views, not the authoritative source.

### 2. Stable Identity Everywhere

All semantically meaningful entities must have stable IDs:

- project
- plates
- objects
- instances
- volumes
- physical filaments
- virtual/mixed filaments
- presets
- derived build outputs

No semantic identity may depend on:

- array position
- plate index
- object export order
- enabled-row order

### 3. Additive Evolution

The schema must evolve by adding fields, parts, and features, not by reinterpreting old positions.

### 4. Unknown Data Must Be Safe

Unknown future fields or parts must not be silently reinterpreted as something else.

Unknown optional data should be ignored and preserved.

Unknown required data should fail closed.

### 5. Separate Intent From Derivatives

Project intent, slice results, and machine toolpaths should be distinct layers.

That makes the format easier to validate, cache, diff, and re-slice.

### 6. Human-Readable, Machine-Validatable

Canonical custom data should use structured JSON parts with published JSON Schemas, not opaque strings.

### 7. Integrity Uses Standard Mechanisms

Package integrity, signatures, and provenance should reuse OPC / 3MF mechanisms where possible.

Ad hoc checksum sidecars are useful for caching and quick validation, but package authenticity should not depend on a proprietary signing model.

## Document Classes

3MF now recommends double-extension naming for different use cases. FullSpectrum should adopt explicit document classes:

- `.project.3mf`
  Canonical authoring / editing project.
- `.build.3mf`
  Prepared build with slice outputs and target-machine context.
- `.toolpath.3mf`
  Toolpath-centric package where G-code or equivalent output is the main artifact.

The same profile can support all three, but the package manifest must declare which class the package is.

## Proposed Package Layout

```text
[Content_Types].xml
_rels/.rels
3D/3dmodel.model

Metadata/fullspectrum/manifest.json
Metadata/fullspectrum/project.json
Metadata/fullspectrum/identity-map.json
Metadata/fullspectrum/plates.json
Metadata/fullspectrum/presets.json
Metadata/fullspectrum/mixed-filaments.json

Metadata/fullspectrum/build/<plate-uuid>/slice.json
Metadata/fullspectrum/build/<plate-uuid>/toolpath.gcode
Metadata/fullspectrum/build/<plate-uuid>/toolpath.sha256

Metadata/fullspectrum/assets/...

docProps/core.xml

Metadata/project_settings.config               # optional legacy projection
Metadata/model_settings.config                 # optional legacy projection
Metadata/slice_info.config                     # optional legacy projection
Metadata/process_settings_<n>.config           # optional legacy projection
Metadata/filament_settings_<n>.config          # optional legacy projection
Metadata/machine_settings_<n>.config           # optional legacy projection
```

Notes:

- `3D/3dmodel.model` remains the standard 3MF geometry root.
- FullSpectrum custom data lives under `Metadata/fullspectrum/`.
- All FullSpectrum custom parts are linked from the package root via relationships.
- All canonical FullSpectrum custom parts are also linked with `MustPreserve`.
- legacy Snapmaker/Bambu-style metadata files are optional compatibility projections only.

## Required Package Relationships

Top-level package relationships should include:

- StartPart to `/3D/3dmodel.model`
- Thumbnail relationship to package thumbnail
- relationship to `/Metadata/fullspectrum/manifest.json`
- `MustPreserve` relationships for every canonical FullSpectrum custom part

Proposed custom relationship types:

- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-manifest`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-project`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-identity-map`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-plates`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-presets`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-mixed-filaments`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-build-slice`
- `https://schemas.fullspectrum.dev/3mf/2026/relationships/fs-build-toolpath`

The exact domain is illustrative. If standardized, it should move to a stable public namespace.

## Required Content Types

Illustrative content types:

- `application/vnd.fullspectrum.manifest+json`
- `application/vnd.fullspectrum.project+json`
- `application/vnd.fullspectrum.identity-map+json`
- `application/vnd.fullspectrum.plates+json`
- `application/vnd.fullspectrum.presets+json`
- `application/vnd.fullspectrum.mixed-filaments+json`
- `application/vnd.fullspectrum.slice+json`
- `text/x.gcode`

## Canonical Parts

### 1. `manifest.json`

This is the package entrypoint for FullSpectrum-aware consumers.

Responsibilities:

- declare document class
- declare package schema version
- declare required and optional features
- enumerate canonical parts
- declare checksums and media types
- declare which part is authoritative for which semantic area

Example:

```json
{
  "kind": "org.fullspectrum.package-manifest",
  "schema_version": "1.0.0",
  "document_class": "project",
  "package_id": "pkg_7f6a3313-2f6d-4e19-9c4a-5f5d8ec1a6d2",
  "created_with": {
    "application": "Snapmaker Orca FullSpectrum",
    "application_version": "0.9.9",
    "profile_version": "1.0.0"
  },
  "features": {
    "required": [
      "fs.project.v1",
      "fs.identity-map.v1"
    ],
    "optional": [
      "fs.mixed-filaments.v1",
      "fs.local-z.v1",
      "fs.slice.v1",
      "fs.toolpath.v1"
    ]
  },
  "parts": [
    {
      "role": "project",
      "path": "/Metadata/fullspectrum/project.json",
      "content_type": "application/vnd.fullspectrum.project+json",
      "required": true,
      "checksum": {
        "algorithm": "sha256",
        "value": "..."
      }
    },
    {
      "role": "mixed-filaments",
      "path": "/Metadata/fullspectrum/mixed-filaments.json",
      "content_type": "application/vnd.fullspectrum.mixed-filaments+json",
      "required": false,
      "checksum": {
        "algorithm": "sha256",
        "value": "..."
      }
    }
  ],
  "authoritative_sources": {
    "project_settings": "/Metadata/fullspectrum/project.json",
    "mixed_filaments": "/Metadata/fullspectrum/mixed-filaments.json",
    "plate_layout": "/Metadata/fullspectrum/plates.json"
  },
  "legacy_projection": {
    "present": true,
    "paths": [
      "/Metadata/project_settings.config",
      "/Metadata/model_settings.config",
      "/Metadata/slice_info.config"
    ]
  }
}
```

### 2. `project.json`

This holds package-wide project intent and configuration, including:

- printer compatibility targets
- process settings
- feature toggles
- preferred presets by stable ID
- generic project metadata
- authoring/session metadata

This should store normalized config as structured objects, not as a flat untyped key dump.

Suggested top-level sections:

- `project`
- `machine_compatibility`
- `process`
- `materials`
- `features`
- `ui_state`
- `compatibility`

### 3. `identity-map.json`

This is the critical glue between:

- transient 3MF numeric IDs
- stable package UUIDs

Without this, editors end up depending on object order or plate order.

Example:

```json
{
  "kind": "org.fullspectrum.identity-map",
  "schema_version": "1.0.0",
  "model_object_bindings": [
    {
      "model_object_id": 5,
      "stable_object_id": "obj_0d7aa2f6-a624-4d5b-a75d-0df6f4d2fdf4"
    }
  ],
  "volume_bindings": [
    {
      "model_object_id": 5,
      "model_volume_id": 2,
      "stable_volume_id": "vol_7b1d83db-3f1d-43ad-b1fc-6f8f72b6f6d0"
    }
  ],
  "build_item_bindings": [
    {
      "build_item_index": 1,
      "stable_instance_id": "inst_99a26356-7b9e-4880-bb7c-f9b0f46f786e"
    }
  ]
}
```

### 4. `plates.json`

This stores plate topology and presentation state as structured records:

- plate IDs
- plate names
- lock state
- object/instance membership
- per-plate process overrides
- thumbnails and derived build part references

Plate identity must use stable UUIDs, not `plate_1`, `plate_2`, etc. Filenames may still include UUIDs or friendly aliases.

### 5. `presets.json`

This stores embedded preset snapshots in a normalized form with stable references.

Instead of positional filenames like `process_settings_1.config`, use:

- stable preset IDs
- preset kind
- preset source
- inheritance info
- normalized config payload

## FullSpectrum Mixed Filaments

### Canonical Part: `mixed-filaments.json`

This replaces `mixed_filament_definitions` as the canonical source of truth.

It should contain:

- physical filament registry
- virtual/mixed filament registry
- stable IDs for all filament-like entities
- blend logic
- cadence logic
- grouped/manual wall patterns
- gradient component sets
- Local-Z controls
- surface bias / expansion data
- UI ordering and tombstone state

### Proposed Structure

```json
{
  "kind": "org.fullspectrum.mixed-filaments",
  "schema_version": "1.0.0",
  "physical_filaments": [
    {
      "id": "fil_5f3cfef5-40f6-4221-9c83-4bcfa25a7bd0",
      "slot_index": 1,
      "display_name": "PLA Red",
      "color": "#FF0000",
      "preset_ref": "preset_filament_red"
    },
    {
      "id": "fil_a2ac8cae-d81c-4e31-9b40-565819d881c7",
      "slot_index": 2,
      "display_name": "PLA Blue",
      "color": "#0000FF",
      "preset_ref": "preset_filament_blue"
    }
  ],
  "virtual_filaments": [
    {
      "id": "mix_4d5d1f5b-98fe-4d13-b4db-96c3a58c0f15",
      "enabled": true,
      "visibility_state": "active",
      "source_kind": "custom",
      "origin": {
        "kind": "pair",
        "component_refs": [
          "fil_5f3cfef5-40f6-4221-9c83-4bcfa25a7bd0",
          "fil_a2ac8cae-d81c-4e31-9b40-565819d881c7"
        ],
        "origin_auto_generated": false
      },
      "blend": {
        "type": "pair_ratio",
        "component_b_percent": 50
      },
      "distribution": {
        "mode": "simple"
      },
      "manual_pattern": null,
      "gradient": null,
      "local_z": {
        "max_sublayers": 0
      },
      "surface_bias": {
        "component_a_offset_mm": 0.0,
        "component_b_offset_mm": 0.0
      },
      "ui": {
        "sort_index": 17
      }
    }
  ]
}
```

### Manual Pattern Representation

Do not persist grouped wall patterns as a flattened comma trick.

Instead use explicit structure:

```json
{
  "manual_pattern": {
    "groups": [
      ["component_a", "component_a", "component_a", "component_a", "component_a", "component_a", "component_a", "component_b"],
      ["component_a", "component_a", "component_a", "component_b", "component_a", "component_a", "component_a", "component_a"]
    ]
  }
}
```

Allowed step tokens:

- `component_a`
- `component_b`
- `physical:<filament-id>`

This is verbose, but safe, explicit, and versionable.

### Gradient Representation

Do not encode gradients as `g123` and `w50/25/25`.

Use explicit component references and numeric weights:

```json
{
  "gradient": {
    "component_refs": [
      "fil_1",
      "fil_2",
      "fil_3"
    ],
    "weights": [50, 25, 25]
  }
}
```

### Distribution Mode Representation

Do not use integers.

Use string enums:

- `simple`
- `layer_cycle`
- `same_layer_pointillisme`
- `height_weighted`
- `local_z`
- future values allowed

Unknown mode values must be handled by feature negotiation, not silent reinterpretation.

### Tombstones

Deleted auto rows are important state and should remain first-class:

```json
{
  "visibility_state": "tombstoned"
}
```

Do not overload this across `enabled`, `deleted`, and `origin_auto` booleans without a semantic wrapper.

## Derived Build Data

Derived build data should be optional and isolated from project intent.

For each plate:

- `build/<plate-uuid>/slice.json`
  Structured slice summary.
- `build/<plate-uuid>/toolpath.gcode`
  Toolpath artifact.
- `build/<plate-uuid>/toolpath.sha256`
  Digest.

Suggested `slice.json` payload:

- target machine id
- nozzle set
- per-plate predicted time
- per-plate predicted mass
- used filament summary
- warnings
- bounding boxes / first-layer summaries
- references to thumbnails and toolpaths

## Integrity And Provenance

If FullSpectrum needs trusted interchange, it should layer on top of existing OPC / 3MF signature support instead of defining a custom signing format.

Recommended rules:

- `manifest.json` checksums are advisory integrity hints for fast validation and caching
- package authenticity should rely on OPC digital signatures
- signatures should cover canonical FullSpectrum parts and any machine-output artifacts included in the package
- unsigned packages remain valid unless a consuming workflow explicitly requires signed input

Optional future canonical part:

- `Metadata/fullspectrum/provenance.json`
  Build provenance, authoring history, source revision, and non-security attestations

This part should never replace OPC signatures for tamper detection.

## Compatibility Model

### Versioning Rules

Every canonical FullSpectrum part must contain:

- `kind`
- `schema_version`

Versioning rules:

- patch version: clarifications or constraints only
- minor version: additive fields / additive parts / additive optional features
- major version: incompatible semantic change

Reader rules:

- same major, newer minor: load and ignore unknown additive fields
- newer major on required part: fail closed
- newer major on optional part: preserve if possible, ignore otherwise

### Feature Negotiation

The manifest declares:

- required features
- optional features

Reader behavior:

- unknown required feature => fail
- unknown optional feature => ignore + preserve

### Unknown Fields

JSON schema policy:

- top-level objects should allow unknown properties
- consumers must ignore unknown fields they do not understand
- editors should preserve unknown fields when rewriting a part they do not semantically normalize

### Unknown Parts

All canonical FullSpectrum custom parts should be attached with `MustPreserve`.

That gives unaware-but-well-behaved editors a chance to keep them during round-trips.

### Vendor Extensions

To keep the profile open without fragmenting it, non-standard vendor additions should follow a strict rule set:

- vendor-specific parts live outside canonical FullSpectrum roles
- vendor-specific features must use a vendor-scoped feature id
- vendor-specific parts must have their own content types and relationship types
- vendor-specific parts may augment canonical semantics, but must not silently redefine them
- if a vendor extension is required to interpret the package correctly, it must be listed under `features.required`

Illustrative path convention:

- `Metadata/extensions/<reverse-dns-vendor>/...`

## Backward Compatibility Strategy

To make adoption realistic, vNext should support dual-write during migration.

### Phase 1

Write both:

- canonical vNext FullSpectrum parts
- current legacy Snapmaker/Bambu projection

Read preference:

1. vNext canonical parts
2. legacy projection fallback

### Phase 2

Continue dual-read indefinitely.

Optionally allow users to disable legacy write for clean packages.

### Legacy Projection Rules

If legacy parts are written:

- they must be explicitly marked as derived compatibility projections in `manifest.json`
- canonical source of truth remains the new FullSpectrum parts
- old consumers keep working
- new consumers never have to infer authority from whichever file they happen to parse first

## Forward Compatibility Strategy

This proposal is more forward compatible than the current string blob because:

- every entity is an object, not a position
- every part is self-described
- every part is versioned
- feature requirements are explicit
- unknown fields do not turn into accidental manual-pattern tokens
- new optional parts can be added without modifying existing parsers

## Suggested Normative Rules

If this ever becomes a public profile, these are good baseline requirements:

- consumers MUST treat `manifest.json` as the canonical FullSpectrum entrypoint
- consumers MUST ignore unknown optional fields
- consumers MUST fail on unknown required features
- editors MUST preserve unknown canonical FullSpectrum parts referenced by `MustPreserve`
- producers MUST use stable UUID-like identifiers for semantic entities
- producers MUST NOT encode semantic meaning solely by array position or filename suffix
- producers MUST NOT use opaque CSV-like strings for canonical structured data
- canonical FullSpectrum parts MUST be UTF-8 JSON
- machine-specific outputs MUST be optional derived artifacts, not required for project validity

## Suggested Standardization Path

To make this credible as an industry-facing standard:

1. Publish the profile and JSON Schemas in a public repository.
2. Define a namespace and relationship/content-type registry.
3. Publish conformance classes:
   - project reader
   - project editor
   - build reader
   - toolpath reader
4. Publish test vectors:
   - minimal project
   - mixed filament project
   - Local-Z project
   - multicolor gradient project
   - legacy dual-write package
5. Publish migration rules from the current Snapmaker/Bambu-like layout.
6. Add a reference validator and a round-trip preservation test suite.

## Bottom Line

The core idea is simple:

- keep standard 3MF for geometry, OPC packaging, relationships, thumbnails, and generic interoperability
- move FullSpectrum semantics into dedicated, versioned, structured custom parts
- use stable IDs instead of positional encoding
- use manifest-driven discovery and explicit feature negotiation
- reuse standard OPC / 3MF signatures for trust-sensitive workflows
- keep legacy files only as a compatibility projection during migration

That gives FullSpectrum something the current package does not really have:

- a canonical schema
- safe extensibility
- predictable compatibility behavior
- a path from "works in our fork" to "can plausibly be published as a real profile"

## Appendix: Mapping From Current Format

Current source:

- `Metadata/project_settings.config`
  Proposed destination: `project.json` and `mixed-filaments.json`
- `mixed_filament_definitions`
  Proposed destination: `mixed-filaments.json`
- `Metadata/model_settings.config`
  Proposed destination: `identity-map.json` plus `plates.json`, with only geometry left in `3D/3dmodel.model`
- `Metadata/slice_info.config`
  Proposed destination: per-plate `slice.json`
- `Metadata/process_settings_<n>.config`, `filament_settings_<n>.config`, `machine_settings_<n>.config`
  Proposed destination: `presets.json` or stable per-preset JSON parts

## References

- 3MF Core Specification v1.3.0: https://3mf.io/spec/core-v1-3-0/
- 3MF Core Specification source: https://github.com/3MFConsortium/spec_core/blob/master/3MF%20Core%20Specification.md
- 3MF specification index: https://3mf.io/spec/
