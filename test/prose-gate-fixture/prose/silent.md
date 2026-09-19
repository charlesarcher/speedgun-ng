# Prose gate silent fixture

Deliberate silent constructs for the prose gate fixtures: every banned
token below sits inside a construct the gate exempts, so nothing reports.

The priority tiers P0–P3 drain in order.

Set the knob `robust: true` before the run.

```text
The design is robust, honestly powerful, and it is in order to be so.
```

    The queue drains -- slowly -- but honestly.

See https://example.com/prose--gate/robust for the note.

Update specs/002-prose--commit-lint/spec.md in the same change.

$ cmake --build --preset=dev

> The design is robust, honestly, and powerful.

The candidate adjusted the dial, a routine calibration.

The review read "robust, honestly powerful" <!-- prose-lint: allow reason="verbatim external review" -->

```text
Robust prose with an invalid marker. prose-lint: allow
```

The tiers P0–P3 remain ordered.

A structural table stays silent, delimiter row included:

| Name | Meaning | Left |
| --- | :--- | ---: |
| tier | priority rank | high |

---

<!--

A quoted block delimited by raw HTML comment markers stays silent.

-->
