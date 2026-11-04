import csv
import json
import time
from datetime import datetime, timezone
from pathlib import Path

import serial

PORT = "/dev/cu.usbmodem11101"  # Update this to the Pico serial port on your computer.
BAUD_RATE = 115200
SAMPLE_SECONDS = 0.5


def now_utc():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def ask(prompt):
    return input(prompt).strip()


# Create a separate directory for each experiment run.
started_at = datetime.now(timezone.utc)
run_id = started_at.strftime("run_%Y%m%dT%H%M%SZ")
run_dir = Path("logs") / run_id
run_dir.mkdir(parents=True)

setpoint_c = float(ask("Pico setpoint temperature in °C (e.g. 23.5): "))
ambient_c = float(ask("Ambient temperature in °C: "))
dut_load = ask("DUT and load description: ")

metadata = {
    "run_id": run_id,
    "started_at_utc": started_at.isoformat(timespec="seconds"),
    "serial_port": PORT,
    "baud_rate": BAUD_RATE,
    "setpoint_c": setpoint_c,
    "ambient_temperature_c": ambient_c,
    "dut_load": dut_load,
    "temperature_source": "Pico dummy_readings (simulated)",
}

(run_dir / "metadata.json").write_text(
    json.dumps(metadata, indent=2, ensure_ascii=False),
    encoding="utf-8",
)

with serial.Serial(PORT, BAUD_RATE, timeout=1) as ser:
    time.sleep(2)  # Wait for the Pico USB serial port to initialize.
    ser.reset_input_buffer()

    # Current firmware expects the setpoint as an integer in hundredths of a degree: 23.5 °C -> 2350.
    command = f"{round(setpoint_c * 100)}\n"
    ser.write(command.encode("ascii"))
    ser.flush()

    # Wait for the OK acknowledgement; DATA lines may arrive first.
    deadline = time.monotonic() + 5
    confirmed_setpoint = None

    with (run_dir / "events.csv").open("w", newline="", encoding="utf-8") as event_file:
        event_writer = csv.writer(event_file)
        event_writer.writerow(["timestamp_utc", "event", "details"])
        event_writer.writerow([now_utc(), "RUN_STARTED", run_id])
        event_writer.writerow([now_utc(), "SETPOINT_SENT", command.strip()])

        while time.monotonic() < deadline:
            line = ser.readline().decode("utf-8", errors="replace").strip()

            if line.startswith("OK "):
                confirmed_setpoint = float(line.split()[1])
                event_writer.writerow([
                    now_utc(), "SETPOINT_CONFIRMED", confirmed_setpoint
                ])
                event_file.flush()
                break

            if line.startswith("NOK"):
                event_writer.writerow([now_utc(), "SETPOINT_REJECTED", line])
                raise RuntimeError(f"Pico rejected the setpoint: {line}")

        if confirmed_setpoint is None:
            event_writer.writerow([now_utc(), "SETPOINT_TIMEOUT", "5 seconds"])
            raise TimeoutError("Did not receive an OK acknowledgement from the Pico.")

        with (run_dir / "measurements.csv").open(
            "w", newline="", encoding="utf-8"
        ) as measurement_file:
            writer = csv.writer(measurement_file)
            writer.writerow([
                "timestamp_utc",
                "temperature_c",
                "setpoint_c",
                "raw_line",
            ])

            print(
                f"Pico acknowledged setpoint {confirmed_setpoint:.2f} °C."
                "Logging started. Press Ctrl+C to stop."
            )

            try:
                while True:
                    line = ser.readline().decode(
                        "utf-8", errors="replace"
                    ).strip()

                    if not line.startswith("DATA:"):
                        if line:
                            event_writer.writerow([now_utc(), "SERIAL_MESSAGE", line])
                            event_file.flush()
                        continue

                    # Current protocol is DATA:<temperature>,<humidity>; the host records temperature only.
                    values = line[5:].split(",")
                    temperature_c = float(values[0])

                    writer.writerow([
                        now_utc(),
                        temperature_c,
                        confirmed_setpoint,
                        line,
                    ])
                    measurement_file.flush()
                    print(f"{temperature_c:.2f} °C")

            except KeyboardInterrupt:
                event_writer.writerow([now_utc(), "RUN_STOPPED", "Ctrl+C"])
                event_file.flush()

print(f"Experiment log saved to: {run_dir}")