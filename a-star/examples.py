import cv2
import numpy as np
import socket
import pyastar
import struct
import json
import base64
from kafka import KafkaProducer, KafkaConsumer
from kafka.errors import KafkaError
import time
from os.path import basename, join, splitext


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

def kafka_produce_message(topic, message):
    producer = KafkaProducer(bootstrap_servers='kafka:9092',
                             max_request_size=15728640)
    producer.send(topic, message)
    producer.flush()


if __name__ == '__main__':
    print("A* App Started", flush=True)

    # Wait for Kafka to be ready
    wait_for_kafka(timeout=60, interval=5)

    print("Subscribing to Kafka topic...", flush=True)
    # Kafka Consumer to receive filenames
    consumer = KafkaConsumer('astar_topic',
                             bootstrap_servers='kafka:9092',
                             auto_offset_reset='earliest',
                             value_deserializer=lambda x: x.decode('utf-8'))

    print("Waiting for filename...", flush=True)
    for message in consumer:
        print("Message received!", flush=True)
        filename = message.value.strip('"')
        print("File name: ", filename)
        MAZE_FPATH = join('mazes', filename)
        OUTP_FPATH = join('solns', '%s_soln.png' % splitext(basename(MAZE_FPATH))[0])
        maze = cv2.imread(MAZE_FPATH)

        if maze is None:
            print('no file found: %s' % (MAZE_FPATH), flush=True)
        else:
            print('loaded maze of shape %r' % (maze.shape[0:2],), flush=True)

        grid = cv2.cvtColor(maze, cv2.COLOR_BGR2GRAY).astype(np.float32)
        grid[grid == 0] = np.inf
        grid[grid == 255] = 1

        assert grid.min() == 1, 'cost of moving must be at least 1'

        # start is the first white block in the top row
        start_j, = np.where(grid[0, :] == 1)
        start = np.array([0, start_j[0]])

        # end is the first white block in the final column
        end_i, = np.where(grid[:, -1] == 1)
        end = np.array([end_i[0], grid.shape[0] - 1])

        t0 = time.time()
        # set allow_diagonal=True to enable 8-connectivity
        path = pyastar.astar_path(grid, start, end, allow_diagonal=False)
        dur = time.time() - t0

        print('done', flush=True)
        if path.shape[0] > 0:
            print('found path of length %d in %.6fs' % (path.shape[0], dur), flush=True)
            maze[path[:, 0], path[:, 1]] = (0, 0, 255)

            # Serialize and send the maze image via Kafka
            _, buffer = cv2.imencode('.png', maze)
            kafka_produce_message('astar_server_topic', buffer.tobytes())
            print("Results sent to TCP Server!", flush=True)
        else:
            print('no path found', flush=True)
