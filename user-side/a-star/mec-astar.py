from io import BytesIO
import socket
import struct
from PIL import Image

def main():
    # TCP server details
    server_ip = '10.0.0.145'  # TCP server's IP
    server_port = 2222       # TCP server's port
    buffer_size = 15000000

    # Connect to the TCP server
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((server_ip, server_port))
        print("Connection successful")
    except socket.error as e:
        print(f"Connection error: {e}")

    # Send filename to the server
    filename = "maze_small.png"
    print("Sent: ", client_socket.send("astar".encode()))
    print("Sent: ", client_socket.send(filename.encode()))

    # First, receive the size of the image
    image_size = struct.unpack(">L", client_socket.recv(4))[0]
    print("Image size: ", image_size)
    # Then, receive the image data
    buffer = b""
    while len(buffer) < image_size:
        data = client_socket.recv(buffer_size)
        if not data:
            break
        buffer += data

    print("Image received, decoding...")
    # Now 'buffer' contains the complete image data
    stream = BytesIO(buffer)
    image = Image.open(stream).convert("RGBA")
    stream.close()
    image.save("received_image1.png")
    image.show()

    print("Sent: ", client_socket.send("maze_large.png".encode()))
    
    # First, receive the size of the image
    image_size = struct.unpack(">L", client_socket.recv(4))[0]
    print("Image size: ", image_size)
    # Then, receive the image data
    buffer = b""
    while len(buffer) < image_size:
        data = client_socket.recv(buffer_size)
        if not data:
            break
        buffer += data

    print("Image received, decoding...")
    # Now 'buffer' contains the complete image data
    stream = BytesIO(buffer)
    image = Image.open(stream).convert("RGBA")
    stream.close()
    image.save("received_image2.png")
    image.show()

    
    print("Image received and saved as 'received_image.png'")

    # Close the connection
    client_socket.close()

if __name__ == "__main__":
    main()