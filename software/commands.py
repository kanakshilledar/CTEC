import serial

MAX_TEMP = 85
MIN_TEMP = -40


# Returns [temperature, humidity] in float format
def read_data(ser):
    while True:
        line = ser.readline().decode().strip()

        if line.startswith("DATA:"):
            return list(map(float, line[5:].split(",")))

        print(f"Ignoring: {line}")

# Sets the temperature with 2 decimals of precision, to avoid floating errors
def set_temperature(ser, setpoint):
    if (float(setpoint) < MIN_TEMP or float(setpoint) > MAX_TEMP):
        print("Setpoint is outside the range")
        return -1
    
    setpoint = round(float(setpoint) * 100)

    ser.write(f"{setpoint}\n".encode())
    for _ in range(10000):
        response = ser.readline().decode()
        if response.startswith("OK"):
            break
        elif response.startswith("DATA:"):
            pass
        else:
            print("Number not valid")
            return -1
    
    print(f"Received answer: {response.replace('\n', '')}")

def main():
    ser = serial.Serial(port='COM3', timeout = 5)
    ser.reset_input_buffer()
    while True:
        data = read_data(ser)
        print(data)
        set_temperature(ser, input())
    ser.close()

if __name__ == "__main__":
    main()