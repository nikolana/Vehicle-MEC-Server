# Use the Ubuntu image as a parent image
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Set the working directory in the container
WORKDIR /usr/src/mpc-kafka

# Copy the MPC app into the container at /usr/src/app
COPY . .

# Install dependencies
RUN apt-get update && apt-get install -y cmake gfortran unzip zlib1g-dev wget file tar build-essential cppad librdkafka-dev pkg-config
RUN ./Model-Predictive-Control/libs/install-ubuntu.sh 

# Install ipopt
RUN ./Model-Predictive-Control/libs/install_ipopt.sh Ipopt-3.12.7

# Build the application
RUN rm -rf Model-Predictive-Control/build/
RUN cd Model-Predictive-Control && ./build.sh

# Add MUMPS library to path
ENV LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}

# Run the application when the container launches
CMD ["./Model-Predictive-Control/build/mpc"]