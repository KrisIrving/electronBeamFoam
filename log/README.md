# Project development log

This directory is the chronological research record for the electronBeamFoam
development programme.

It exists for a different purpose from `docs/` and `validation/`:

- `docs/` describes the current technical state of the solver;
- `validation/` stores accepted/rejected numerical baselines and machine-readable
  results;
- `log/` records **how and why** the project reached that state: hypotheses,
  alternatives, failed attempts, evidence, decisions, limitations and next
  questions.

That distinction is deliberate. A paper normally needs both the final method
and the reasoning that justified it. The development log preserves the second
part instead of reconstructing it months later from Git history.

## Files

- `METHODOLOGY.md` — project-level method for developing and validating the
  solver.
- `DEVELOPMENT_HISTORY.md` — retrospective technical timeline from the
  laserbeamFoam baseline to the present EPBF calibration stage.
- `DECISIONS.md` — compact decision register: accepted, rejected and deferred
  modelling/numerical choices.
- `EXPERIMENT_LEDGER.md` — simulation/test ledger with purpose, changed
  variable, evidence and conclusion.
- `entries/` — dated narrative entries for important development episodes.
- `entries/TEMPLATE.md` — template for future entries.

## Entry rule

For any change that can affect physics, numerics, performance or interpretation,
record the following:

1. **Question** — what uncertainty are we trying to remove?
2. **Hypothesis** — what do we expect and why?
3. **Single controlled change** — what changed relative to the accepted
   baseline?
4. **Evidence** — case, files, metrics and commit.
5. **Decision** — accept, reject, defer or investigate.
6. **Limitations** — what this test does *not* prove.
7. **Next action** — the next gate.

This is the same discipline used in the calibration campaign: a short run may
be a numerical/performance gate without being a quantitative physical
validation.

## Evidence hierarchy

Prefer, in order:

1. committed machine-readable validation result;
2. compact run summary / comparison output;
3. raw solver log and CSV diagnostics;
4. interpretation recorded in a dated log entry.

When a later result supersedes an earlier interpretation, do not erase the old
entry. Add a new entry and mark the earlier conclusion as superseded.

## Writing use

For papers/theses/reports:

- use `docs/` for the final formulation;
- use `validation/` for quantitative tables;
- use `log/` to reconstruct the development rationale, rejected alternatives,
  sensitivity-study design, numerical safeguards and limitations.

The log is intentionally more candid than publication prose.
