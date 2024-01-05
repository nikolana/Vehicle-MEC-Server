# Use an official Python runtime as a parent image
FROM python:3.9-slim

# Set the working directory in the container
WORKDIR /usr/src/tcp-kafka

# Copy the TCP server source code into the container at /usr/src/tcp-kafka
COPY . .

# Install any needed packages specified in requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

# Make port 2222 available to the world outside this container
EXPOSE 2222

# Run app.py when the container launches
CMD ["python", "./tcp.py"]
