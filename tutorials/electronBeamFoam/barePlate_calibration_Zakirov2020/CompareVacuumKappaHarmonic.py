#!/usr/bin/env python3
from pathlib import Path
import json
import math
import sys
import traceback

case = Path(__file__).resolve().parent
root = case / "vacuumFidelity" / "V1c_harmonic"
legacy_root = case / "vacuumFidelity" / "V1_kappa"

labels = ("h1", "h0p1", "h0p01")

def load(path):
    if not path.is_file():
        raise RuntimeError(f"Missing {path}")
    d=json.loads(path.read_text())
    return d

def pct(new, old):
    if abs(old) < 1e-30:
        return 0.0 if abs(new) < 1e-30 else float("inf")
    return 100.0*(new-old)/old

def metrics(a,b):
    out={}
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
        old=float(a[group][key]); new=float(b[group][key])
        out[name]=(old,new,pct(new,old))
    return out

def show_pair(title,a,b):
    print(title)
    m=metrics(a,b)
    for name,(old,new,d) in m.items():
        if name in ("W","D","L"):
            scale=1e6; unit="um"
        elif name=="V":
            scale=1e9; unit="mm^3"
        elif name in ("fusion Tmax","melt Tmax"):
            scale=1.0; unit="K"
        elif name=="recoil":
            scale=1e-3; unit="kPa"
        elif name=="Qevap":
            scale=1.0; unit="W"
        else:
            scale=1.0; unit="m3"
        print(f"  {name:11s}: {old*scale:.6g} -> {new*scale:.6g} {unit}  delta={d:+.3f}%")
    print()
    return {k:v[2] for k,v in m.items()}

def main():
    data={label:load(root/label/"summary.json") for label in labels}

    print("Vacuum Fidelity V1c: harmonic face conductivity")
    print("  cell kappa remains alpha*k_m + (1-alpha)*k_void")
    print("  thermal Laplacian coefficient interpolation = harmonic")
    print("  scales: 1x -> 0.1x -> 0.01x")
    print()

    legacy_path=legacy_root/"k1"/"summary.json"
    if legacy_path.is_file():
        legacy=load(legacy_path)
        show_pair("Same cell k_void=1x: legacy linear-face -> harmonic-face", legacy, data["h1"])

    show_pair("Harmonic: h1 -> h0p1", data["h1"], data["h0p1"])
    final_delta=show_pair("Harmonic: h0p1 -> h0p01", data["h0p1"], data["h0p01"])

    final=data["h0p01"]
    ev_frac=100.0*float(final["evaporation"]["equivalentMetalVolume_m3"])/float(final["fusion_zone"]["volume_m3"])
    q_frac=100.0*float(final["evaporation"]["evaporationPower_W"])/765.0

    plateau=(
        abs(final_delta["W"]) <= 0.25
        and abs(final_delta["D"]) <= 0.25
        and abs(final_delta["L"]) <= 0.25
        and abs(final_delta["V"]) <= 0.5
        and abs(final_delta["fusion Tmax"]) <= 0.5
        and abs(final_delta["melt Tmax"]) <= 0.5
        and abs(final_delta["recoil"]) <= 5.0
        and abs(final_delta["Qevap"]) <= 2.0
        and ev_frac <= 0.5
        and q_frac <= 1.0
    )

    print("V1c plateau gate (harmonic 0.1x -> 0.01x):")
    print("  W/D/L <=0.25%, V <=0.5%, Tmax <=0.5%, recoil <=5%, Qevap <=2%")
    print(f"  final evap/fusion = {ev_frac:.6f}%")
    print(f"  final Qevap/absorbed = {q_frac:.6f}%")
    print()

    if plateau:
        print("  PASS: harmonic face interpolation removes material sensitivity")
        print("  over the tested low-k range.")
        print("  Recommended production choice: harmonic face interpolation with")
        print("  k_void scale=0.1x, the largest conductivity inside the plateau.")
        print("  Proceed to V2 rho/nu sensitivity using that thermal baseline.")
    else:
        print("  REVIEW: harmonic face interpolation has not yet produced a")
        print("  sufficiently insensitive low-k thermal solution.")
        print("  Do not proceed to V2; isolate thermal-void treatment further.")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SystemExit:
        raise
    except Exception as exc:
        print(f"ERROR: V1c comparison failed: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise SystemExit(2)
