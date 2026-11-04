# Host Communication

This demo sends a temperature setpoint to the Pico over USB serial and logs the Pico response and simulated temperature readings. The current firmware acknowledges the setpoint but does not yet control chamber temperature.

## Setup (macOS)

From the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install pyserial
```

Find the Pico serial port with `ls /dev/cu.usbmodem*`, then update `PORT` in `software/log.py`.

## Run

With the virtual environment active, run from the repository root:

```bash
python software/log.py
```

Enter the requested setpoint in °C, ambient temperature, and DUT/load description. The host sends the setpoint as hundredths of a degree (for example, 23.5 °C is sent as `2350`) and waits for an `OK` acknowledgement. Press `Ctrl+C` to stop logging.

Each run creates a timestamped directory under `logs/` with `metadata.json`, `measurements.csv`, and `events.csv`. These generated logs and the local `.venv/` are excluded from Git by the root `.gitignore`. Temperature readings are simulated until real sensors are integrated. An `OK` response confirms command receipt only; it does not confirm that the chamber reached the setpoint.
