
#pragma once
// SOO/Controls constants for Water-Cooled Scroll Chiller + Tower
// Generated: 2025-08-31

// ===== Access/roles (example bitmask) =====
enum Role { VIEWER=0, OPERATOR=1, ADMIN=2, BAS=3 };

// ===== I2C / DAC =====
#define MCP4728_ADDR            0x60    // adjust if different
#define VDC_FAILSAFE_LOW        3.0f    // <3.0 V -> force full speed until flow proven
#define VDC_MIN_NORMAL          4.0f    // minimum normal operating command for pumps
#define FAN_MIN_VDC             3.0f    // tower fan minimum command

// ===== Compressor VFD limits =====
#define COMP_MIN_HZ             3.0f
#define COMP_MAX_HZ             150.0f

// ===== Timers (seconds) =====
#define AHU_STAGE_S             10      // AHU fan staging interval
#define AHU_PROOF_DELAY_S       5       // open proof window after command
#define AHU_PROOF_S             10      // total time allowed to prove airflow (Î”P); tighten as needed

#define EVAP_PROOF_S            30      // evap pump proof window
#define COND_PROOF_S            30      // condenser pump proof window

#define MIN_ON_S                120     // generic min on-time (fan/pumps) â€” tune as needed
#define MIN_OFF_S               120     // generic min off-time (fan/pumps)

#define COMP_STOP_TO_START_S    300     // compressor anti-recycle (5 min)
#define COMP_START_TO_START_S   900     // compressor start-to-start (15 min)
#define COMP_MAX_STARTS_PER_HR  6       // rolling window cap

#define EEV_CLOSE_S             10      // pre-start EEV close duration
#define PUMP_OVERRUN_S          90      // evap pump overrun after compressor stops
#define LOW_HEAD_S              30      // time below head min before action
#define NO_FLOW_S               10      // time at near-zero flow while at high command

// ===== Temperatures (Â°F) =====
#define LCHWS_MIN_F             42.0f   // setpoint limits exposed to users
#define LCHWS_MAX_F             52.0f
#define LCHWS_DEFAULT_F         46.0f

#define FREEZE_WARN_F           36.0f
#define FREEZE_TRIP_F           34.0f
#define MIN_APPROACH_F          2.0f
#define OAT_LOCKOUT_F           35.0f   // initial low ambient lockout threshold (tune for site)

// ===== Head pressure (psi) =====
#define HEAD_TARGET_PSI         220.0f  // initial target; commissioning tune 200â€“250
#define HEAD_BAND_PSI           20.0f   // +/- band around target
#define HEAD_MIN_PSI            200.0f  // do not operate persistently below this
#define HEAD_MAX_PSI            250.0f  // soft ceiling; HP safety trip is separate

// ===== Flow / Proof thresholds (examples) =====
#define AHU_DP_PROOF_INWC       0.10f   // required Î”P rise across fan (in. w.c.) â€” set per hardware
#define EVAP_DP_PROOF_PSI       1.0f
#define COND_DP_PROOF_PSI       1.0f
#define FLOWMETER_MIN_GPM       0.1f    // non-zero threshold
#define FLOWMETER_INCREASE_GPM  0.5f    // increase vs baseline to count as "proven"
#define PUMP_HIGH_CMD_V         7.5f    // "high command" threshold used by zero-flow check

// ===== Flags (digital inputs) =====
#define DI_TOWER_LEVEL_OK       1       // example mapping; wire to actual DI
#define DI_AHU_FLOW_SWITCH      2
#define DI_EVAP_FLOW_SWITCH     3
#define DI_COND_FLOW_SWITCH     4

// ===== Helper macros =====
#define WITHIN_BAND(x, tgt, band) (((x) >= (tgt)-(band)) && ((x) <= (tgt)+(band)))

// ===== Notes =====
// - COMP_MAX_STARTS_PER_HR is a secondary guard; with COMP_START_TO_START_S=900, effective max is â‰¤4/h.
// - FAN_MIN_VDC and VDC_MIN_NORMAL are enforced clamps on analog outputs.
// - Freeze protection uses both absolute LCHWT and approach to catch sensor bias.
// - Low OAT lockout prevents excessive tower cooling/low head; adjust OAT_LOCKOUT_F per climate.
