# Distributed CAN-Based Pipe Organ Control Architecture

## 1. The Epiphany

The core realization:

> We do not need a centralized controller in the hot path of note processing.

Instead:
- Divisions are naturally independent
- Keyboards generate note intent
- Windchests decide local actuation
- CAN provides reliable transport

This allows us to:
- Remove MIDI from critical pipe control
- Eliminate single-point note routing bottlenecks
- Gain robustness through state-based messaging

---

## 2. High-Level Architecture

### System Components

#### Keyboard Controllers (per manual/pedal)
- Scan physical key switches
- Maintain authoritative key state
- Apply couplers (duplicate note state to other divisions)
- Broadcast **state bitmaps**, not edge events

#### Windchest Controllers
- Own local ranks
- Maintain local stop state
- Subscribe to relevant division bitmaps
- Merge inputs and actuate pipes

#### Master Controller (Raspberry Pi)
- NOT in hot note path
- Owns:
  - Pistons
  - UI / REST API
  - Registration authority (stops/couplers)
  - Virtual ranks (FluidSynth / Hauptwerk)
- Broadcasts authoritative stop/coupler state

---

## 3. Core Design Principles

### 3.1 State, Not Events

Instead of:
- note on/off events

We use:
- full **key state bitmaps**

Benefits:
- Self-healing (no missed edges)
- Stateless recovery
- Deterministic behavior

---

### 3.2 Division-Based Independence

Each division operates independently:
- Windchests only care about relevant divisions
- No global coordination required for note flow

---

### 3.3 Distributed Actuation, Centralized Authority

- **Distributed:**
  - Note handling
  - Pipe actuation

- **Centralized:**
  - Stop state
  - Couplers
  - Pistons

---

## 4. CAN Bus Design

### 4.1 Payload Constraints
- Classic CAN: **8 bytes max**
- Perfect fit for:
  - 61-note bitmap = 8 bytes

---

### 4.2 Message Types

#### Division Bitmap Broadcast
Sent by keyboard controllers:

```
DIV_STATE
- division_id
- source_id
- bitmap (up to 8 bytes)
```

---

#### Stop State (Authoritative)
Sent by master:

```
STOP_STATE
- stop_id
- state (on/off)
```

---

#### Coupler State
Sent by master:

```
COUPLER_STATE
- coupler_id
- state
```

---

#### Snapshot Request
Sent by windchest:

```
SNAPSHOT_REQ
- division_id
```

---

#### Snapshot Response
Sent by keyboard:

```
SNAPSHOT
- division_id
- bitmap
```

---

## 5. Bitmap Strategy (Critical)

### 5.1 DO NOT send merged division state

Instead, send **per-source bitmaps**.

Examples:
- Great physical keys
- Great→Pedal coupler output
- Swell→Great coupler output

---

### 5.2 Windchest Merge Logic

Each windchest computes:

```
effective_division = OR(all relevant bitmaps)
```

Then per pipe:

```
pipe_on = stop_enabled AND effective_division[note]
```

---

## 6. Coupler Handling

Handled entirely in keyboard controllers:

- Keyboard generates:
  - its own division bitmap
  - additional bitmaps for coupled divisions

Example:
- Great→Pedal enabled
- Great keyboard sends:
  - Great bitmap
  - Pedal contribution bitmap

---

## 7. Stop Handling

- Stopboards send raw events
- Master resolves and rebroadcasts authoritative state
- Windchests store only relevant stops

---

## 8. Recovery & Synchronization

### 8.1 Automatic Recovery
Every bitmap message is authoritative → self-healing

### 8.2 Explicit Recovery
- Snapshot request/response
- "All notes off" command
- Startup broadcast

---

## 9. Virtual Ranks

Master acts as a **virtual windchest**:

- Subscribes to division bitmaps
- Applies stop state
- Outputs via MIDI / FluidSynth

---

## 10. Why This Works

### Eliminates MIDI Failure Mode
- No silent bit flips
- CAN ensures delivery

### Removes Timing Sensitivity
- No dependency on event ordering

### Scales Cleanly
- More ranks = more listeners, not more routing complexity

### Robust to Failure
- Any node can resync instantly

---

## 11. Final Architecture Summary

### Data Flow

```
Keyboard → CAN (bitmaps)
        → Windchests (local actuation)
        → Master (virtual ranks)

Master → CAN (stops/couplers)
```

---

## 12. Key Design Rules

1. **Never rely on edge events alone**
2. **Bitmaps are authoritative**
3. **Windchests merge, not interpret**
4. **Keyboard owns couplers**
5. **Master owns registration**
6. **One CAN bus is sufficient**

---

## 13. Future Extensions

- CAN FD (optional)
- Per-rank diagnostics
- Load balancing across buses
- Distributed watchdog / health monitoring

---

## 14. Bottom Line

> Distribute note handling. Centralize musical intent.

This architecture gives you:
- reliability
- scalability
- simplicity in the hot path
- flexibility for future expansion
