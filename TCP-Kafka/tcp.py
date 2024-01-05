import socket
import time
import threading
from kafka import KafkaProducer, KafkaConsumer, TopicPartition
from kafka.errors import KafkaError
import json
import numpy as np
import struct   


def check_kafka_connection():
    """Check if Kafka connection can be established."""
    try:
        producer = KafkaProducer(bootstrap_servers='kafka:9092')
        producer.send('connection_test_topic', b'test').get(timeout=10)
        return True
    except KafkaError:
        return False

def wait_for_kafka(timeout=60, interval=5):
    """Wait for Kafka to become available within the given timeout."""
    start_time = time.time()
    while time.time() - start_time < timeout:
        if check_kafka_connection():
            print("Kafka connection established.", flush=True)
            return
        print("Waiting for Kafka to become available...", flush=True)
        time.sleep(interval)
    raise Exception("Kafka connection could not be established within the timeout period.")

class KafkaClientApplication:
    def __init__(self, kafka_server, host, port):
        self.host = host
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind((host, port))
        self.server_socket.listen()
        self.client_sockets = {}

        self.mpc_producer = KafkaProducer(bootstrap_servers=kafka_server,
                                      value_serializer=lambda v: v.encode('utf-8'))

        self.astar_producer = KafkaProducer(bootstrap_servers=kafka_server,
                                      value_serializer=lambda v: json.dumps(v).encode('utf-8'))
        
        # Consumer for tcp_server_topic
        self.tcp_consumer = KafkaConsumer(bootstrap_servers=kafka_server,
                                          group_id='tcp_group',
                                          auto_offset_reset='earliest',
                                          fetch_max_bytes=15728640)
        self.tcp_consumer.assign([TopicPartition('astar_server_topic', 0)])  # A* to server direction

        # Consumer for mpc_topic
        self.mpc_consumer = KafkaConsumer(bootstrap_servers=kafka_server,
                                          group_id='mpc_group',
                                          auto_offset_reset='earliest',
                                          fetch_max_bytes=15728640)
        self.mpc_consumer.assign([TopicPartition('mpc_server_topic', 0)])  # MPC to server direction

    def handle_user_connection(self, client_socket, address, user_type):
        """Handle communication with the connected user."""
        self.client_sockets[user_type] = client_socket
        while True:
            received_data = client_socket.recv(300).decode()
            if received_data:
                print(f"Received from {user_type} user {address}: {received_data}", flush=True)
                self.send_data_to_kafka(received_data, user_type)

    def start_user_listener(self):
        """Listen for incoming user connections."""
        print(f"Listening for user connections on {self.host}:{self.port}")
        while True:
            client_socket, address = self.server_socket.accept()
            print(f"User connected from {address}", flush=True)
            user_type = client_socket.recv(5).decode()  # Expecting 'astar' or 'mpc' as first message
            print(f"User type: {user_type}", flush=True)
            thread = threading.Thread(target=self.handle_user_connection, args=(client_socket, address, user_type))
            thread.start()

    def send_data_to_kafka(self, data, user_type):
        """Send data to the appropriate Kafka topic based on user type."""
        topic = 'astar_topic' if user_type == 'astar' else 'mpc_topic'
        print(f"Sending data to {topic}...", flush=True)
        if topic == 'astar_topic':
            self.astar_producer.send(topic, data)
            self.astar_producer.flush()
        elif topic == 'mpc_topic':
            self.mpc_producer.send(topic, data)
            self.mpc_producer.flush()

    def start_listening_for_kafka_messages(self):
        """Listen for messages on Kafka topics."""
        print("Polling for Kafka topics...", flush=True)
        while True:
            tcp_messages = self.tcp_consumer.poll(timeout_ms=1000)
            for _, messages in tcp_messages.items():
                for message in messages:
                    self.forward_results_to_user(message.value, 'astar')

            mpc_messages = self.mpc_consumer.poll(timeout_ms=5000)
            for _, messages in mpc_messages.items():
                for message in messages:
                    self.forward_results_to_user(message.value, '-mpc-')



    def forward_results_to_user(self, data, user_type):
        """Forward the data to the respective user."""
        client_socket = self.client_sockets.get(user_type)
        if client_socket:
            try:
                if user_type == 'astar':
                    image_size = len(data)
                    client_socket.sendall(struct.pack(">L", image_size))  # Send the size of the image
                    client_socket.sendall(data)  # Send the image data
                elif user_type == '-mpc-':
                    print("Sending MPC results to the user...", flush=True)
                    print("Sent: ", client_socket.send(data), flush=True)
            except socket.error as e:
                print(f"Socket error: {e}", flush=True)

if __name__ == "__main__":
    kafka_server = 'kafka:9092'  # Kafka server address
    host = '0.0.0.0'  # Host to listen for user connections
    port = 2222  # Port to listen for user connections

    # Wait for Kafka to be ready
    wait_for_kafka(timeout=60, interval=5)

    app = KafkaClientApplication(kafka_server, host, port)

    # Start listening for user connections
    user_listener_thread = threading.Thread(target=app.start_user_listener)
    user_listener_thread.start()

    # Start listening for Kafka messages
    kafka_listener_thread = threading.Thread(target=app.start_listening_for_kafka_messages)
    kafka_listener_thread.start()
