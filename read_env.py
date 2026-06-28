import os

# --- Automatically install python-dotenv if missing ---
try:
    from dotenv import load_dotenv
except ImportError:
    import pip
    pip.main(['install', 'python-dotenv'])
    from dotenv import load_dotenv

# Load variables from your local .env file
load_dotenv()

from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()

# Fetch environment variables or fall back to defaults
wifi_ssid = os.getenv("WIFI_SSID", "DEFAULT_SSID")
wifi_pass = os.getenv("WIFI_PASS", "DEFAULT_PASS")
mqtt_server = os.getenv("MQTT_SERVER", "localhost")
mqtt_user = os.getenv("MQTT_USER", "guest")
mqtt_pass = os.getenv("MQTT_PASS", "guest")

# Fix: Double-escape the quotes so GCC receives them as literal string constants
env.Append(CPPDEFINES=[
    ("WIFI_SSID", f'\\"{wifi_ssid}\\"'),
    ("WIFI_PASS", f'\\"{wifi_pass}\\"'),
    ("MQTT_SERVER", f'\\"{mqtt_server}\\"'),
    ("MQTT_USER", f'\\"{mqtt_user}\\"'),
    ("MQTT_PASS", f'\\"{mqtt_pass}\\"')
])