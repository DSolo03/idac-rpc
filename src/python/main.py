import socket
import os
import time
import pymem
from pymem.process import inject_dll_from_path
import sys

import configparser

import enums
from pypresence import Presence

config = configparser.ConfigParser()

if getattr(sys, 'frozen', False):
    application_path = os.path.dirname(sys.executable)
else:
    application_path = os.getcwd()

config_path = os.path.join(application_path, 'config.ini')

config.read(config_path)

DEFAULT_PORT = 5555
DEFAULT_PROCESS = "GameProject-Win64-Shipping.exe"
DEFAULT_CLIENT_ID = '1462194051263758649'

processName = config.get('inject', 'process', fallback = DEFAULT_PROCESS)
dllName = "idac-rpc.dll"

discordClientID = config.get("discord", "clientID", fallback = DEFAULT_CLIENT_ID)

try:
    discordClientID = int(discordClientID)
except ValueError:
    socketPort = DEFAULT_CLIENT_ID

socketIP = "127.0.0.1"
socketPort = config.get('socket', 'port', fallback = DEFAULT_PORT)

try:
    socketPort = int(socketPort)
except ValueError:
    socketPort = DEFAULT_PORT

def inject():
    try:
        dll_path = os.path.abspath(dllName)
        
        pm = pymem.Pymem(processName)
        print(f"Found {processName} (PID: {pm.process_id})")
        
        if not os.path.exists(dll_path):
            print(f"Error: {dll_path} not found!")
            return False

        inject_dll_from_path(pm.process_handle, dll_path) # type: ignore
        print(f"DLL Injected successfully!")
        return True
    except Exception as e:
        print(f"Injection failed: {e}")
        return False

def listen():

    rpc = Presence(discordClientID)
    try:
        rpc.connect()
        print("Connected to Discord RPC")
    except Exception as e:
        print(f"Could not connect to Discord: {e}")
        rpc = None

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((socketIP, socketPort))
    
    print(f"Listening on {socketIP}:{socketPort}...")

    last_state = -1
    race_start_time = None

    try:
        while True:
            data, _ = sock.recvfrom(1024)
            raw_state = data.decode('utf-8', errors='ignore').split(",") 
            
            if len(raw_state) < 5:
                continue

            try:
                state_id = int(raw_state[0])
                mode_id  = int(raw_state[1])
                map_id   = int(raw_state[2])
                dir_id   = int(raw_state[3])
                time_id  = int(raw_state[4])

                if rpc:
                    if state_id == 4:
                        if last_state != 4:
                            race_start_time = time.time()
                        
                        mode_str = enums.GameMode.get_display(mode_id)
                        map_str  = enums.Course.get_display(map_id)
                        dir_str  = enums.Direction.get_display(dir_id)
                        time_str = enums.Time.get_display(time_id)

                        rpc.update(
                            details=f"{mode_str}",
                            state=f"{map_str} ({dir_str})",
                            large_text=f"{map_str}",
                            small_text=f"{time_str}",
                            start=race_start_time,
                            large_image="logo"
                        )
                        if last_state != 4:
                            print(f"[+] Entered Race: {map_str} ({dir_str})")
                    
                    elif state_id == 3:
                        rpc.update(details="Main Menu", state="Selecting a course...", large_image="logo")
                        race_start_time = None
                    
                    elif state_id == 1:
                        rpc.update(details="Title Screen", state="Attract Mode", large_image="logo")
                        race_start_time = None
                        
                    else:
                        state_name = enums.GlobalState.get_display(state_id)
                        rpc.update(details=f"{state_name}", state="Please wait...", large_image="logo")
                        race_start_time = None

                    last_state = state_id

            except Exception as rpc_err:
                print(f"RPC Update Error: {rpc_err}")

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        if rpc: rpc.close()
        sock.close()

if __name__ == "__main__":
    if inject():
        listen()