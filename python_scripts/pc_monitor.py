import time
import psutil
import requests

from command_sync import start_command_sync

ESP32_URL = "http://192.168.4.1/api/stats"
MAX_PROCESSES = 5


def get_processes():
    processes = []

    for proc in psutil.process_iter(["pid", "name"]):
        try:
            proc.cpu_percent(interval=None)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue

    time.sleep(0.5)

    for proc in psutil.process_iter(["pid", "name"]):
        try:
            name = proc.info["name"] or "Unknown"
            cpu = proc.cpu_percent(interval=None)
            cpu = max(0, min(100, round(cpu)))

            processes.append({
                "name": name[:19],
                "cpu": cpu
            })

        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue

    processes.sort(key=lambda p: p["cpu"], reverse=True)
    return processes[:MAX_PROCESSES]


def main():
    print("ESP32 PC Monitor started.")
    print("Press Ctrl+C to stop.\n")

    start_command_sync()

    while True:
        try:
            cpu = round(psutil.cpu_percent(interval=0.5))
            ram = round(psutil.virtual_memory().percent)
            processes = get_processes()

            # GPU monitoring can be added later.
            gpu = 0

            data = {
                "cpu": cpu,
                "ram": ram,
                "gpu": gpu,
                "processes": processes
            }

            response = requests.post(
                ESP32_URL,
                json=data,
                timeout=3
            )
            response.raise_for_status()

            print(
                f"CPU: {cpu:3d}% | "
                f"RAM: {ram:3d}% | "
                f"GPU: {gpu:3d}%"
            )

            print("Top processes:")

            for i, proc in enumerate(processes, start=1):
                print(
                    f"  {i}. {proc['name']:<19} "
                    f"{proc['cpu']:3d}%"
                )

            print("ESP32: Updated successfully\n")

        except requests.RequestException as error:
            print(f"ESP32 connection error: {error}")

        except KeyboardInterrupt:
            print("\nPC Monitor stopped.")
            break

        time.sleep(0.5)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nPC Monitor stopped.")