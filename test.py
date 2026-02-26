import requests

# Test Credentials
BOT_TOKEN = '8736583227:AAFGcR8NWgHvw1a3h_mnezfWk5gU_RGZfqc'
CHAT_ID = '982391689'
URL = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"

payload = {'chat_id': CHAT_ID, 'text': 'Aiko testing the bot... answer me!'}

try:
    r = requests.post(URL, json=payload)
    print(f"Status Code: {r.status_code}")
    print(f"Response Body: {r.text}")
except Exception as e:
    print(f"Connection Error: {e}")