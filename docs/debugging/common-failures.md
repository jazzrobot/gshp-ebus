# Common Failures

## No traffic captured

Likely causes:

- the interface is not producing a valid logic-level output
- UART settings are wrong
- the bus signal needs inversion or different conditioning
- the tap is connected to the wrong pair

First actions:

- return to the bench setup and verify the interface output
- double-check the physical wiring
- confirm the receive pin sees transitions before debugging decoding

## Gibberish bytes or framing errors

Likely causes:

- poor signal conditioning
- wrong baud or framing
- excessive noise or grounding problems

First actions:

- inspect the interface design before touching firmware
- verify isolation and grounding assumptions
- keep capture firmware simple until the electrical side is stable

## Traffic appears, then drops out

Likely causes:

- buffer overruns in firmware
- unstable power to the listener
- intermittent wiring or poor strain relief

First actions:

- add error counters and watchdog logging
- shorten or tidy temporary wiring
- check PoE and interface power stability separately

## Heat pump or Tado behaves oddly after the tap is attached

Treat this as a safety event, not a debugging curiosity.

First actions:

1. disconnect the tap
2. return the wiring to the original arrangement
3. confirm normal behaviour is restored
4. do not reconnect until the interface design has been reviewed

## Decoding does not line up with observed temperatures

Likely causes:

- the field mapping is wrong
- scaling or signedness is wrong
- the frame belongs to a different device or operating mode

First actions:

- compare against known values at a known time
- log enough context around changes
- avoid publishing a sensor until the mapping is repeatable
