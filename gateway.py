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

last_alert_time = 0
def send_telegram_alert():
    print("\n[GATEWAY] 🚨 Sentinel Value Recognized! Transmitting in background...")
    payload = {
        'chat_id': CHAT_ID,
        'text': '🚨 EMERGENCY! Yao Xiang has fallen! Immediate assistance required!'
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
                        # Check for the Sentinel Value
                        if line == TRIGGER_PHRASE:
                            current_time = time.time()
                            
                            # 10-second software debounce to prevent API bans
                            if current_time - last_alert_time > 10:
                                last_alert_time = current_time
                                print("\n[GATEWAY] ⏱️ Cooldown activated for 10 seconds...")
                                
                                # Fire the API call in a BACKGROUND THREAD
                                # This prevents the serial buffer from freezing!
                                threading.Thread(target=send_telegram_alert, daemon=True).start()
                                
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