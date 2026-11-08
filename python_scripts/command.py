import ctypes
import threading
import time

import requests
import comtypes
from ctypes import POINTER, cast
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

ESP32_BASE = "http://192.168.4.1"
TOKEN = "change-me"
POLL_SECONDS = 0.25


def set_volume(level):
    level = max(0, min(100, int(level)))

    comtypes.CoInitialize()

    try:
        device = AudioUtilities.GetSpeakers()

        endpoint = getattr(device, "EndpointVolume", None)

        if endpoint is None:
            interface = device.Activate(
                IAudioEndpointVolume._iid_,
                CLSCTX_ALL,
                None
            )

            endpoint = cast(
                interface,
                POINTER(IAudioEndpointVolume)
            )

        endpoint.SetMasterVolumeLevelScalar(
            level / 100.0,
            None
        )

    finally:
        comtypes.CoUninitialize()


def lock_pc():
    return bool(ctypes.windll.user32.LockWorkStation())


def _worker():
    session = requests.Session()

    headers = {
        "X-Auth-Token": TOKEN
    }

    last_volume = None
    last_lock_seq = None

    while True:
        try:
            response = session.get(
                ESP32_BASE + "/api/command",
                headers=headers,
                timeout=1.5
            )

            response.raise_for_status()
            data = response.json()

            volume = data.get("volume")

            if volume is not None:
                volume = max(0, min(100, int(volume)))

                # First reading establishes the baseline.
                if last_volume is not None and volume != last_volume:
                    try:
                        set_volume(volume)
                        print(f"[sync] Laptop volume set to {volume}%")
                        last_volume = volume

                    except Exception as error:
                        print(f"[sync] Volume error: {error}")

                else:
                    last_volume = volume

            lock_seq = data.get("lock_seq")

            if lock_seq is not None:
                lock_seq = int(lock_seq)

                # Lock only when the sequence number increases.
                if last_lock_seq is not None and lock_seq > last_lock_seq:
                    print("[sync] Locking laptop...")

                    if lock_pc():
                        print("[sync] Lock request sent successfully.")
                    else:
                        print("[sync] Failed to request laptop lock.")

                last_lock_seq = lock_seq

        except (requests.RequestException, ValueError) as error:
            print(f"[sync] Connection error: {error}")
            last_volume = None
            last_lock_seq = None

        except Exception as error:
            print(f"[sync] Error: {error}")

        time.sleep(POLL_SECONDS)


def start_command_sync():
    thread = threading.Thread(
        target=_worker,
        daemon=True
    )

    thread.start()
    return thread


if __name__ == "__main__":
    print("ESP32 Windows Command Sync started.")
    print("Press Ctrl+C to stop.")

    start_command_sync()

    try:
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nCommand synchronization stopped.")