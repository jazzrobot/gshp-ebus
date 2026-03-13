# Live eBUS Install Checklist

Use this checklist before attaching anything to the live heat pump bus.

## Before touching the plant

1. Confirm the MVP is still passive and read-only.
2. Confirm the interface has been bench-validated.
3. Confirm the transmit path is not connected.
4. Prepare a simple rollback plan so the tap can be removed immediately.
5. Take photos of the original wiring.

## Electrical safety

1. Isolate mains power to the appliance before opening covers or touching internal terminals.
2. Confirm you are working on the low-voltage communication terminals only.
3. Do not route new wiring through mains compartments unless the appliance design explicitly allows it.
4. Keep temporary wiring tidy, strain-relieved, and away from hot or moving parts.

## Bus safety

1. Connect the tap in parallel with the existing eBUS pair.
2. Use the isolated interface only. Do not connect raw ESP32 pins to the bus.
3. Keep the listener power source separate from the bus unless the interface was designed for bus power.
4. Leave any transmit-capable conductor disconnected for the MVP.

## First power-up

1. Restore power.
2. Confirm the heat pump and Tado boot normally.
3. Confirm no new faults or warnings appear.
4. Start capture and watch for stable traffic.
5. Leave the system under observation for the first 10 to 15 minutes.

## Stop immediately if

- a new fault appears
- the heat pump display behaves differently from normal
- Tado loses control or connectivity
- the bus traffic disappears unexpectedly after the tap is attached
- any part of the interface becomes warm or unstable

## Rollback

1. Disconnect the tap.
2. Restore the original wiring exactly as photographed.
3. Confirm normal heating behaviour returns.
4. Document what happened before attempting another install.
