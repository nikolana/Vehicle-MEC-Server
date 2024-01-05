# Vehicle MEC Server Testbed
## Project Overview

This project serves as a testbed for the Vehicle Mobile Edge Computing (MEC) Server. It utilizes an open-source Udacity simulator, designed to test Model Predictive Control (MPC) software in simulated vehicle environments. The architecture is orchestrated using Docker Compose, ensuring isolated and interconnected service deployment.
Docker Compose Overview

The `docker-compose.yml` file outlines the multi-container Docker application. Key components include:

**mpc-container**: Utilizes the `mpc-kafka` image, integrating with the MEC network and depending on Kafka      initialization.\
**astar-container**: Runs the `astar-app` image, part of the MEC network, and also depends on Kafka initialization.\
**zookeeper**: Uses `wurstmeister/zookeeper` image, crucial for managing Kafka, exposed on port 2181.
**init-kafka**: Built from `wurstmeister/kafka`, this initializes Kafka topics necessary for the application's messaging.\
**kafka**: Also using `wurstmeister/kafka`, it handles messaging, configured for larger message sizes and relies on Zookeeper.\
**tcp-gateway**: Using the `tcp-kafka image`, it bridges TCP communication, exposed on port 2222, and links to both A* and MPC containers.

All services are connected through the `mec-network`, a Docker bridge network.

## Getting Started

### Prerequisites

    - Docker
    - Docker Compose
	
### Installation

1.  Clone the repository:

   	```bash

	git clone [repository-url] 
	```

2. Navigate to the project directory:

	```bash

	cd [project-directory]
	```

3. Run the Docker Compose command to build and start the containers:

	```bash
	docker-compose up -d
	```

### Usage

After successful deployment, the testbed is ready for simulations. To interact with the Vehicle MEC Server, use the TCP gateway exposed on port 2222.
