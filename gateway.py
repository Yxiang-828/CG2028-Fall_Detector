import serial
import requests
import time
import threading

# --- CONFIGURATION ---
COM_PORT = 'COM3'      # <<< CHANGE THIS TO YOUR ACTUAL COM PORT
BAUD_RATE = 115200 

BOT_TOKEN = '8736583227:AAFGcR8NWgHvw1a3h_mnezfWk5gU_RGZfqc'
CHAT_ID = '982391689'
TELEGRAM_URL = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"

# The exact Sentinel Value printed by the STM32 FSM transition
TRIGGER_PHRASE = "___SEND_TELEGRAM_ALERT___"
DISARM_PHRASE = "--- SYSTEM DISARMED (2 presses) ---"
ARM_PHRASE = "--- SYSTEM ARMED (2 presses) ---"
MANUAL_ALARM_PHRASE = "!!! MANUAL ALARM TRIGGERED (3 presses) !!!"
RESET_ALARM_PHRASE = "--- ALARM RESET (1 press) ---"

# Independent cooldown timers for each event type
last_alert_times = {
    'FALL': 0,
    'DISARM': 0,
    'ARM': 0,
    'MANUAL': 0,
    'RESET': 0
}

def send_telegram_alert(message="🚨 EMERGENCY! Yao Xiang has fallen! Immediate assistance required!"):
    from datetime import datetime
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    full_message = f"{message}\n⏰ Time: {timestamp}"
    
    print(f"\n[GATEWAY] 🚨 Transmitting in background: {full_message}")
    payload = {
        'chat_id': CHAT_ID,
        'text': full_message
    }
    try:
        # 5-second timeout ensures the thread dies cleanly if Wi-Fi drops
        response = requests.post(TELEGRAM_URL, json=payload, timeout=5)
        if response.status_code == 200:
            print("\n[GATEWAY] ✅ Telegram alert sent successfully!")
        else:
            print(f"\n[GATEWAY] ❌ Failed to send. Status: {response.status_code}")
    except Exception as e:
        print(f"\n[GATEWAY] ❌ Network Error: {e}")

def main():
    global last_alert_time
    print("--- CG2028 SENTINEL GATEWAY ---")
    
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
        print(f"[GATEWAY] ✅ Listening on {COM_PORT}... Drop the board to test!\n")
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        current_time = time.time()
                        
                        # Check for the Sentinel Value
                        if line == TRIGGER_PHRASE:
                            # 10-second debounce specific to real falls
                            if current_time - last_alert_times['FALL'] > 10:
                                last_alert_times['FALL'] = current_time
                                print("\n[GATEWAY] ⏱️ Fall alarm engaged. Cooldown activated for 10 seconds...")
                                
                                # Fire the API call in a BACKGROUND THREAD
                                threading.Thread(target=send_telegram_alert, daemon=True).start()
                                
                        elif line == MANUAL_ALARM_PHRASE:
                            # 5-second debounce specific to manual alarm presses
                            if current_time - last_alert_times['MANUAL'] > 5:
                                last_alert_times['MANUAL'] = current_time
                                print("\n[GATEWAY] ⏱️ Panic alarm engaged. Cooldown activated for 5 seconds...")
                                
                                threading.Thread(target=send_telegram_alert, args=("🆘 MANUAL PANIC ALARM TRIGGERED BY YAO XIANG! 🆘",), daemon=True).start()
                        
                        elif line == DISARM_PHRASE:
                            # 5-second debounce specific to disarming
                            if current_time - last_alert_times['DISARM'] > 5:
                                last_alert_times['DISARM'] = current_time
                                print("\n[GATEWAY] ⏱️ Disarm logged. Cooldown activated for 5 seconds...")
                                
                                # Custom message for disarm
                                threading.Thread(target=send_telegram_alert, args=("Oi, Yao Xiang has disabled the detector!",), daemon=True).start()
                                
                        elif line == ARM_PHRASE:
                            # 5-second debounce specific to arming
                            if current_time - last_alert_times['ARM'] > 5:
                                last_alert_times['ARM'] = current_time
                                print("\n[GATEWAY] ⏱️ Arming logged. Cooldown activated for 5 seconds...")
                                
                                threading.Thread(target=send_telegram_alert, args=("🛡️ System Armed: Fall Detector is now monitoring.",), daemon=True).start()
                                
                        elif line == RESET_ALARM_PHRASE:
                            # 5-second debounce specific to reset button
                            if current_time - last_alert_times['RESET'] > 5:
                                last_alert_times['RESET'] = current_time
                                print("\n[GATEWAY] ⏱️ Reset logged. Cooldown activated for 5 seconds...")
                                
                                threading.Thread(target=send_telegram_alert, args=("Oi, fall detected, but Yao Xiang terminated the alarm!",), daemon=True).start()
                                
                        else:
                            # Echo the normal STM32 terminal output
                            print(f"[STM32]: {line}") 
                                
                except Exception:
                    pass 
                    
    except serial.SerialException:
        print(f"\n[CRITICAL ERROR] Could not open {COM_PORT}.")
        print("-> Make sure TeraTerm/PuTTY is CLOSED so Python can use the port!")
    except KeyboardInterrupt:
        print("\n[GATEWAY] Shutting down.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == '__main__':
    main()