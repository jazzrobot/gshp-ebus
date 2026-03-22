# 2026-03-22 Tado Perturbation Checklist

Use this checklist for the next short, labelled capture run.

Goal:

- identify which `1F->...` families reflect Tado-side intent or state
- separate controller-originated changes from normal plant response
- keep each intervention isolated enough to be recognisable in the capture

## Before you start

- leave the live collector running
- keep the ESP listener and Ethernet path unchanged
- make sure the system is in a normal steady state before the first action
- note the wall-clock time before each action
- if possible, let the system settle for at least `2-3` minutes between actions

## High-priority families to watch

These are the best candidates based on the overnight run and the labelled compressor cycle.

### Primary targets

- `AA 1F 08 B5 11 01 00`
  - strongest current candidate for controller-side state
- `AA 1F 08 B5 1A 04 05`
  - clearly reacted to compressor run state
- `AA 1F 15 B5 24 06 02`
  - dominant fast-changing family

### Supporting targets

- `AA 1F 08 B5 11 01 01`
  - likely mirror of a known Vaillant operational-data block
- `AA 1F 15 B5 24 05 03`
  - secondary fast `1F->15` loop

### Control families for comparison

- `AA 10 08 B5 11 01 01`
- `AA 10 76 B5 11 01 01`
- `AA 10 08 B5 10 09 00`
- `AA 10 76 B5 10 09 00`

These help tell us whether a change is a Tado-side decision, a plant-side response, or both.

## Recommended test order

Do these one at a time.

### Test 0: Tado "test connection to heat pump"

Action:

- run the Tado HPO X built-in `test connection to heat pump` feature

Why this is especially valuable:

- it is an explicitly Tado-originated action
- it should generate bus traffic even if there is little or no thermal plant response
- that makes it one of the best available ways to isolate controller-side intent from normal background polling

Timing:

- note the exact wall-clock time when you press start
- note when the test reports success, failure, or completion
- if the app shows intermediate stages, note those too
- let the capture run for at least `2` minutes afterwards

Watch for:

- any immediate change in `AA 1F 08 B5 11 01 00`
- any burst or temporary mode change in `AA 1F 08 B5 1A 04 05`
- any unusual short-lived `1F->15` changes outside the normal repeating pattern
- whether the `10->...` Vaillant anchor families react at all

Interpretation:

- if only `1F->...` families move, that strongly suggests a Tado-side protocol exchange
- if `10->...` families follow immediately, that suggests the test is requesting or confirming data from the heat-pump side rather than just exercising the app

### Test 1: Raise heating setpoint

Action:

- increase the Tado room setpoint by `+1 C`

Timing:

- note the wall-clock time at the moment you press confirm
- wait `3` minutes

Watch for:

- an immediate change in `AA 1F 08 B5 11 01 00`
- a rapid change in `AA 1F 08 B5 1A 04 05`
- any shift in the next `AA 1F 15 B5 24 06 02` cycle
- slower follow-on changes in the `10->08` and `10->76` operational families

### Test 2: Return setpoint to baseline

Action:

- put the Tado setpoint back to its original value

Timing:

- note the exact time
- wait `3` minutes

Watch for:

- reversal or partial reversal in the same `1F->...` families
- whether the plant-side `10->...` families lag the Tado-side changes

### Test 3: Change heating mode

Action:

- if available, switch between a normal heating mode and a clearly different mode
- for example: on to off, schedule to manual, or comfort to eco

Timing:

- note the exact time
- wait `3` minutes

Watch for:

- any abrupt bit-like or flag-like changes in `AA 1F 08 B5 11 01 00`
- whether `AA 1F 08 B5 1A 04 05` changes state without an immediate temperature ramp

### Test 4: Toggle DHW

Action:

- if exposed in Tado, toggle domestic hot water once

Timing:

- note the exact time
- wait `3-5` minutes

Watch for:

- any change that appears in `1F->...` first and only later in `10->...`
- whether the slower `~60 s` Vaillant families react as well

## Suggested event log format

Write a plain text note while you do the run.

Example:

```text
2026-03-22 20:14 baseline stable
2026-03-22 20:15 raise Tado setpoint from 19.0 C to 20.0 C
2026-03-22 20:18 restore Tado setpoint from 20.0 C to 19.0 C
2026-03-22 20:22 change heating mode to off
2026-03-22 20:25 change heating mode back to heat
2026-03-22 20:29 DHW on
2026-03-22 20:33 DHW off
```

## Capture handling

After the run:

- keep the raw collector log untouched
- save a short tail snapshot covering the whole perturbation window
- add a notes file beside it with the exact action list

Recommended naming:

- `ebus-YYYY-MM-DD-tado-perturbation-tail.log`
- `ebus-YYYY-MM-DD-tado-perturbation.notes.txt`

## What success looks like

A useful result is not necessarily a large temperature change.

Success is:

- a repeatable immediate change in one or more `1F->...` families right after a Tado action
- followed by a slower or separate response in the `10->...` Vaillant anchor families

That would let us start distinguishing:

- Tado intent
- Tado state
- Vaillant response
- plant telemetry
