
# Sequence of Operations (SOO) — Water‑Cooled Scroll Chiller with Cooling Tower
*Revision: 2025‑08‑31 | Scope: single compressor, single tower fan, no pump lead/lag*

## Roles & Access
- **Viewer**: read‑only values (can’t change setpoints).
- **Operator**: can change modes (Auto/Off), acknowledge alarms, and edit setpoints **only if no active alarms**.
- **Admin** or **BAS**: full write access, manual overrides, Test mode.
- Setpoints editable via **BAS**, **Admin**, or **Operator** (the latter when no alarms). **Viewer** sees setpoints but cannot change them.
- All data trending stored locally for **24 hours** and exposed to BAS.

## Key Permissives & Limits
- **AHU demand permissive**: ≥1 AHU cooling call proven.
- **Tower basin level permissive**: water level switch **OK** (level control handled by separate controller).
- **Freeze protection** (lockout): leaving chilled‑water temp (LCHWT) < `FREEZE_WARN_F` → warn; < `FREEZE_TRIP_F` or approach < `MIN_APPROACH_F` for `FREEZE_TRIP_S` → **trip**.
- **Low OAT lockout**: if OAT < `OAT_LOCKOUT_F` **and** head pressure below `HEAD_MIN_PSI` for `LOW_HEAD_S`, disable tower fan(s) and raise condenser target until head recovers; if prolonged, inhibit compressor start.
- **Compressor start limits**: 
  - **Anti‑recycle** = `COMP_STOP_TO_START_S` (5 min). 
  - **Start‑to‑Start** = `COMP_START_TO_START_S` (15 min). 
  - **Max starts per hour** = 6 (rolling window). *Note: With a 15‑min start‑to‑start, the effective max is ≤4/h; the 6/h ceiling is a secondary guard.*
- **Minimum on/off** timers applied to compressor/tower fan/pumps per constants below.

## VFD/DAC Rules
- **AHU pump** and **Evap pump** commanded via **I²C DAC (MCP4728)**. 
- If AHU or Evap pump command **drops below 3.0 VDC**, force **failsafe full speed** until flow is proven; normal minimum operating command is **4.0 VDC**.
- **Tower fan** minimum command = **3.0 VDC**.
- **Compressor VFD**: min **3 Hz**, max **150 Hz** (subject to OEM limits).

## Start Sequence (Normal Operation)
1. **Pre‑checks** (continuous): comms OK (I²C, VFD), sensors healthy, tower level OK, no lockouts active, timers eligible.
2. **AHU fans**: 
   - When commanded, stage **every 10 s**.
   - **Airflow proof** = duct/fan ΔP using inlet & outlet pressure sensors. Proof window opens **5 s** after start command; if ΔP < threshold within `AHU_PROOF_S`, **fault that AHU** (no global lockout).
3. **AHU pump (I²C DAC)**:
   - Start when ≥1 AHU is proven. Command to full (≥4 VDC), verify within `EVAP_PROOF_S` equivalents:
     - **Differential pressure flow switch**, **suction vs discharge ΔP**, and **flowmeter increase** must all prove.
   - On success, release to automatic VFD command via DAC. On failure, **warn + stop AHU pump** (AHUs faulted).
4. **Chiller enable**:
   - Requires AHU demand proven, pre‑checks OK, no compressor lockout, timers eligible.
5. **Evaporator pump (I²C DAC)**:
   - Command ≥4 VDC; **proof within 30 s** via ΔP **flow switch**, suction vs discharge ΔP, **and** flowmeter increase. Failure → **chiller alarm/lockout** (AHUs unaffected).
   - On success, release to auto modulation.
6. **Sand filter pump**: start **immediately after Evap pump proof** (motor feedback only unless ΔP taps exist).
7. **Condenser water pump**: start **30 s after Sand filter starts**; proof within **30 s** using **flow switch** and **flowmeter increase**. Failure → **chiller alarm/lockout**.
8. **Cooling tower fan**:
   - Enable on condenser flow proof. Enforce **min 3.0 VDC**.
   - Control objective: maintain **head pressure** near target band (`HEAD_TARGET_PSI` ± `HEAD_BAND_PSI`). Initial target 220 psi; tunable **200–250 psi**.
   - Apply min on/off timers and low‑ambient logic to avoid over‑cooling/low head.
9. **Refrigerant circuit prep**:
   - Command **EEVs closed (0%) for 10 s** to home/avoid floodback.
10. **Compressor start**:
   - Check permissives (evap & cond flows proven, EEV homed, timers OK). Ramp VFD from min Hz upward.
   - Enforce **anti‑recycle**, **start‑to‑start**, and **6/h** limit. Abort on HP/LPS/freeze/flow loss.
11. **Automatic modulation**:
   - **Compressor VFD** tracks **LCHWS setpoint** (BAS/Admin/Operator‑editable; Viewer read‑only; range **42–52 °F**).
   - **EEV** controls **evaporator superheat** (target **8–12 °F**) with rate limits and anti‑hunt.
   - **Tower fan** modulates to keep head within band. If head too low at min fan, raise condenser water target temporarily.
12. **Normal shutdown**:
   - On no AHU demand or satisfied chilled‑water temp for hold time: ramp compressor down; stop tower fan & condenser pump with timers; keep **Evap pump overrun** for purge; AHU pump follows AHU demand.

## Protections & Trips
- **High pressure**: immediate trip, manual reset.
- **Low suction / freeze**: trip, manual reset.
- **Evap/Cond flow loss**: trip, manual reset.
- **Sensor plausibility**: inhibit start (identify bad sensor).
- **I²C DAC / VFD comms**: alarm, inhibit compressor until cleared.
- **Zero‑flow while high command**: if flowmeter ≈ 0 while pump command ≥ `PUMP_HIGH_CMD_V` for `NO_FLOW_S`, raise **No‑Flow Alarm**.

## Modes
- **Auto / Off / Manual** per subsystem (Compressor, Evap pump, Cond pump, Tower fan). Manual overrides via **Admin** or **BAS** only; safeties remain enforced.
- **Test Mode**: available to **BAS/Admin** regardless of alarms; **Operator** only if **no active alarms**. Test mode time‑boxed and logged.

## Schedules & BAS
- **Occupied/Unoccupied** schedule with BAS override.
- BAS can adjust all setpoints and limits within admin ranges; expose full point list: permissives, proofs, modes, timers, setpoints, readbacks, run‑hours, and last 24 h trends.

## Initial Tuning Targets (adjust in commissioning)
- **LCHWS**: 42–52 °F, default 45–46 °F.
- **Superheat**: 8–12 °F (start at 10 °F).
- **Head pressure**: start at **220 psi** target with ±20 psi band; allowable 200–250 psi envelope.
