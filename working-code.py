import paho.mqtt.client as mqtt
import firebase_admin
from firebase_admin import credentials, db
import json
from datetime import datetime

# Initialize the Firebase app with Realtime Database
cred = credentials.Certificate("/home/kavish/Downloads/sit210-project-a83fd-firebase-adminsdk-jm86y-8fe123c70e.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://sit210-project-a83fd-default-rtdb.firebaseio.com/'
})

# Reference to the 'latestData' document
ref = db.reference('sensorData/latestData')

# Callback function for when a message is received
def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()
    print(f"Received {payload} from {topic} topic")

    try:
        # Parse the received JSON data
        data = json.loads(payload)

        # Generate a timestamp
        timestamp = datetime.utcnow().isoformat()  # UTC timestamp in ISO format

        # Update Firebase Realtime Database with labeled data
        ref.update({
            'latitude': data['latitude'],
            'longitude': data['longitude'],
            'altitude': data['altitude'],
            'temperature': data['temperature'],
            'humidity': data['humidity'],
            'velocity': data['velocity'],
            'timestamp': timestamp
        })

        print("Data saved to Firebase Realtime Database.")

    except json.JSONDecodeError:
        print("Failed to decode JSON")
    except Exception as e:
        print(f"Failed to process data: {e}")

# MQTT setup
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect("broker.hivemq.com", 1883, 60)
print("Connected successfully to MQTT broker.")
mqtt_client.subscribe("transport/sensorData")  # Subscribe to sensor topic
mqtt_client.loop_start()

# Keep the script running
try:
    while True:
        pass
except KeyboardInterrupt:
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
