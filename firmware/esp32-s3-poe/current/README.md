# Current Listener Build

This directory is reserved for the current canonical passive-listener firmware once code lands.

The first build should:

- initialise only the receive side of the chosen interface
- timestamp incoming bytes or frames
- expose error counters in the log output
- fail safely if the input stream is malformed or absent
