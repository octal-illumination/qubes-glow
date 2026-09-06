# Researched Ideas

## [Complete] KWin 5.27 effect metadata service-type key
- **What was asked:** Will KWin recognise the effect if its embedded metadata uses
  the modern top-level `ServiceTypes` key instead of legacy `X-KDE-ServiceTypes`?
- **Why the search was done:** The build-time JSON only had
  `"ServiceTypes": ["KWin/Effect"]`; it was unclear whether KWin's loader would
  match it (the canonical built-in effects use `X-KDE-ServiceTypes`).
- **Summary of Findings:** Inspected KF5 `kpluginmetadata.h` (Fedora 37, 5.108).
  Line 57 documents `ServiceTypes → serviceTypes()`. KWin's `EffectLoader` filters
  plugins via `serviceTypes().contains("KWin/Effect")`, so the modern key IS
  recognised. No rebuild required. (An external `metadata.json` with both keys was
  also deployed for belt-and-suspenders.)
- **Final Decision & Rationale:** Keep the embedded `ServiceTypes` key; the effect
  is discoverable. Rejected adding `X-KDE-ServiceTypes` to the embedded JSON as
  unnecessary churn.
