#!/usr/bin/env python3
from pathlib import Path
import json

case = Path(__file__).resolve().parent
root = case / "vacuumFidelity" / "V1_kappa"
baseline_path = root / "k1" / "summary.json"

if not baseline_path.is_file():
    raise SystemExit(f"Missing {baseline_path}")

base = json.loads(baseline_path.read_text())
candidates = []
for label in ("k0p1", "k0p01"):
    p = root / label / "summary.json"
    if not p.is_file():
        raise SystemExit(f"Missing {p}")
    candidates.append(json.loads(p.read_text()))

def pct(new, old):
    if abs(old) < 1e-30:
        return 0.0 if abs(new) < 1e-30 else float("inf")
    return 100.0*(new-old)/old

print("Vacuum Fidelity V1: numerical-void thermal conductivity sensitivity")
print("  all cases: 0.5 mm, 296 K, 900 W, 3 m/s, simpleXZ, 48 ranks")
print("  only gas/numerical-void poly_kappa is scaled")
print()

primary_pass = True
secondary_pass = True

for cand in candidates:
    scale = cand["void_kappa_scale"]
    print(f"Candidate k_void scale = {scale:g}x")

    deltas = {}
    for label,key in (
        ("W","width_m"),
        ("D","depth_m"),
        ("L","length_m"),
        ("V","volume_m3"),
        ("fusion Tmax","peakT_K"),
    ):
        old = base["fusion_zone"][key]
        new = cand["fusion_zone"][key]
        d = pct(new,old)
        deltas[label]=d

        factor = 1e6 if key in ("width_m","depth_m","length_m") else (1e9 if key=="volume_m3" else 1.0)
        unit = "um" if key in ("width_m","depth_m","length_m") else ("mm^3" if key=="volume_m3" else "K")
        print(f"  {label:11s}: {old*factor:.6g} -> {new*factor:.6g} {unit}  delta={d:+.3f}%")

    for label,key,unit in (
        ("melt Tmax","Tmax_K","K"),
        ("recoil","recoilMax_Pa","Pa"),
    ):
        old=base["melt_pool"][key]
        new=cand["melt_pool"][key]
        d=pct(new,old)
        deltas[label]=d
        print(f"  {label:11s}: {old:.6g} -> {new:.6g} {unit}  delta={d:+.3f}%")

    old_ev=base["evaporation"]["equivalentMetalVolume_m3"]
    new_ev=cand["evaporation"]["equivalentMetalVolume_m3"]
    dev=pct(new_ev,old_ev)
    fusion_v=cand["fusion_zone"]["volume_m3"]
    evap_fraction=100.0*new_ev/fusion_v if fusion_v>0 else float("inf")

    old_q=base["evaporation"]["evaporationPower_W"]
    new_q=cand["evaporation"]["evaporationPower_W"]
    dq=pct(new_q,old_q)

    print(f"  evap volume : {old_ev:.6e} -> {new_ev:.6e} m3  delta={dev:+.3f}%")
    print(f"  evap/fusion : {evap_fraction:.6f}%")
    print(f"  Qevap       : {old_q:.6g} -> {new_q:.6g} W  delta={dq:+.3f}%")
    if cand.get("clock_time_s") and base.get("clock_time_s"):
        print(f"  clock       : {base['clock_time_s']:.0f} -> {cand['clock_time_s']:.0f} s")

    case_primary = (
        abs(deltas["W"]) <= 0.5
        and abs(deltas["D"]) <= 0.5
        and abs(deltas["L"]) <= 0.5
        and abs(deltas["V"]) <= 1.0
        and abs(deltas["fusion Tmax"]) <= 1.0
        and abs(deltas["melt Tmax"]) <= 1.0
    )
    # Evaporation is a small nonlinear secondary quantity. Avoid rejecting the
    # void model solely because a tiny baseline changes by a large relative
    # percentage; require its absolute importance to remain bounded.
    case_secondary = (
        evap_fraction <= 0.5
        and new_q/765.0*100.0 <= 1.0
    )

    print(f"  primary metal-solution gate = {'PASS' if case_primary else 'REVIEW'}")
    print(f"  evaporation absolute gate  = {'PASS' if case_secondary else 'REVIEW'}")
    print()

    primary_pass = primary_pass and case_primary
    secondary_pass = secondary_pass and case_secondary

print("V1 decision:")
if primary_pass and secondary_pass:
    print("  PASS: melt-pool predictions are insensitive to a 100x reduction")
    print("  in numerical-void thermal conductivity over the tested range.")
    print("  Proceed to V2 density/viscosity regularisation sensitivity.")
else:
    print("  REVIEW: numerical-void thermal transport materially affects at least")
    print("  one predeclared gate; inspect the individual deltas before V2.")
