#!/usr/bin/env python3
from pathlib import Path
import json

case = Path(__file__).resolve().parent
root = case / "vacuumFidelity" / "V1_kappa"

labels = ("k0p01", "k0p001", "k0p0001")
data = {}
for label in labels:
    p = root / label / "summary.json"
    if not p.is_file():
        raise SystemExit(f"Missing {p}")
    data[label] = json.loads(p.read_text())

def pct(new, old):
    if abs(old) < 1e-30:
        return 0.0 if abs(new) < 1e-30 else float("inf")
    return 100.0*(new-old)/old

def compare(a_label, b_label):
    a=data[a_label]
    b=data[b_label]

    metrics=[]
    for group,key,name in (
        ("fusion_zone","width_m","W"),
        ("fusion_zone","depth_m","D"),
        ("fusion_zone","length_m","L"),
        ("fusion_zone","volume_m3","V"),
        ("fusion_zone","peakT_K","fusion Tmax"),
        ("melt_pool","Tmax_K","melt Tmax"),
        ("melt_pool","recoilMax_Pa","recoil"),
        ("evaporation","evaporationPower_W","Qevap"),
        ("evaporation","equivalentMetalVolume_m3","evap volume"),
    ):
        old=a[group][key]
        new=b[group][key]
        metrics.append((name, old, new, pct(new,old)))

    return metrics

print("Vacuum Fidelity V1b: k_void -> 0 convergence extension")
print("  tested scales: 0.01x -> 0.001x -> 0.0001x")
print("  physical k_void: 4e-4 -> 4e-5 -> 4e-6 W/(m K)")
print()

pairs=(("k0p01","k0p001"),("k0p001","k0p0001"))
all_metrics={}
for a,b in pairs:
    print(f"{a} -> {b}")
    metrics=compare(a,b)
    all_metrics[(a,b)]={name:d for name,_,_,d in metrics}
    for name,old,new,d in metrics:
        if name in ("W","D","L"):
            scale=1e6; unit="um"
        elif name=="V":
            scale=1e9; unit="mm^3"
        elif name in ("fusion Tmax","melt Tmax"):
            scale=1.0; unit="K"
        elif name=="recoil":
            scale=1.0; unit="Pa"
        elif name=="Qevap":
            scale=1.0; unit="W"
        else:
            scale=1.0; unit="m3"
        print(
            f"  {name:11s}: {old*scale:.6g} -> {new*scale:.6g} {unit}  "
            f"delta={d:+.3f}%"
        )

    ev=b["evaporation"]["equivalentMetalVolume_m3"]
    fv=b["fusion_zone"]["volume_m3"]
    q=b["evaporation"]["evaporationPower_W"]
    print(f"  evap/fusion absolute = {100.0*ev/fv:.6f}%")
    print(f"  Qevap/absorbed       = {100.0*q/765.0:.6f}%")
    print()

# Plateau is assessed on the final decade (0.001x -> 0.0001x). If this
# passes, use the larger 0.001x conductivity as the production regularisation:
# it is already inside the low-k plateau and is numerically less extreme.
m=all_metrics[("k0p001","k0p0001")]
final=data["k0p0001"]
ev_frac=100.0*final["evaporation"]["equivalentMetalVolume_m3"]/final["fusion_zone"]["volume_m3"]
q_frac=100.0*final["evaporation"]["evaporationPower_W"]/765.0

plateau=(
    abs(m["W"]) <= 0.25
    and abs(m["D"]) <= 0.25
    and abs(m["L"]) <= 0.25
    and abs(m["V"]) <= 0.5
    and abs(m["fusion Tmax"]) <= 0.5
    and abs(m["melt Tmax"]) <= 0.5
    and abs(m["recoil"]) <= 5.0
    and abs(m["Qevap"]) <= 2.0
    and ev_frac <= 0.5
    and q_frac <= 1.0
)

print("V1b plateau gate (0.001x -> 0.0001x):")
print("  geometry W/D/L <= 0.25%, V <= 0.5%")
print("  Tmax <= 0.5%, recoil <= 5%, Qevap <= 2%")
print("  absolute evaporation remains <=0.5% fusion volume and <=1% absorbed power")
print()
if plateau:
    print("  PASS: low-k numerical-void thermal solution is converged.")
    print("  Recommended production regularisation: k_void scale = 0.001x")
    print("  (4e-5 W/(m K)); use the largest conductivity inside the plateau.")
    print("  Proceed to V2 density/viscosity sensitivity using this baseline.")
else:
    print("  REVIEW: the last tested conductivity decade is not yet converged.")
    print("  Do not proceed to V2 or select a production k_void yet.")
