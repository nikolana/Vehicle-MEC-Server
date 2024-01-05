#include <math.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <librdkafka/rdkafkacpp.h>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "MPC.h"
#include "json.hpp"

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.rfind("}]");
  if (found_null != string::npos) {
    return "";
  } else if (b1 != string::npos && b2 != string::npos) {
    return s.substr(b1, b2 - b1 + 2);
  }
  return "";
}

// Evaluate a polynomial.
double polyeval(Eigen::VectorXd coeffs, double x) {
  double result = 0.0;
  for (int i = 0; i < coeffs.size(); i++) {
    result += coeffs[i] * pow(x, i);
  }
  return result;
}

// Fit a polynomial.
// Adapted from
// https://github.com/JuliaMath/Polynomials.jl/blob/master/src/Polynomials.jl#L676-L716
Eigen::VectorXd polyfit(Eigen::VectorXd xvals, Eigen::VectorXd yvals,
                        int order) {
  assert(xvals.size() == yvals.size());
  assert(order >= 1 && order <= xvals.size() - 1);
  Eigen::MatrixXd A(xvals.size(), order + 1);

  for (int i = 0; i < xvals.size(); i++) {
    A(i, 0) = 1.0;
  }

  for (int j = 0; j < xvals.size(); j++) {
    for (int i = 0; i < order; i++) {
      A(j, i + 1) = A(j, i) * xvals(j);
    }
  }

  auto Q = A.householderQr();
  auto result = Q.solve(yvals);
  return result;
}

// convert from map coordinate to car coordinates
void map2car(double px, double py, double psi, const vector<double>& ptsx_map, const vector<double>& ptsy_map,
             Eigen::VectorXd & ptsx_car, Eigen::VectorXd & ptsy_car){

  for(size_t i=0; i< ptsx_map.size(); i++){
    double dx = ptsx_map[i] - px;
    double dy = ptsy_map[i] - py;
    ptsx_car[i] = dx * cos(-psi) - dy * sin(-psi);
    ptsy_car[i] = dx * sin(-psi) + dy * cos(-psi);
  }
}

bool check_kafka_connection() {
    std::string brokers = "kafka:9092";
    std::string errstr;
    std::string topicName = "connection_test_topic";

    // Create Kafka producer configuration
    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("metadata.broker.list", brokers, errstr);

    RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        delete conf;
        return false;
    }

    RdKafka::Topic *topic = RdKafka::Topic::create(producer, topicName, NULL, errstr);
    if (!topic) {
        std::cerr << "Failed to create topic: " << errstr << std::endl;
        delete producer;
        delete conf;
        return false;
    }

    // Send a single message
    RdKafka::ErrorCode resp = producer->produce(topic, RdKafka::Topic::PARTITION_UA, 
                                                RdKafka::Producer::RK_MSG_COPY, 
                                                const_cast<char *>("test"), 4, 
                                                NULL, NULL);

    // Wait for the message to be delivered
    if (producer->flush(1000) != RdKafka::ERR_NO_ERROR) {
        delete topic;
        delete producer;
        delete conf;
        return false;
    }

    delete topic;
    delete producer;
    delete conf;
    return true;
}

void wait_for_kafka(int timeout = 60, int interval = 5) {
    auto start = std::chrono::high_resolution_clock::now();
    int elapsed_seconds = 0;

    while (elapsed_seconds < timeout) {
        if (check_kafka_connection()) {
            std::cout << "Kafka connection established." << std::endl;
            return;
        }
        std::cout << "Waiting for Kafka to become available..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        auto now = std::chrono::high_resolution_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
    }
    throw std::runtime_error("Kafka connection could not be established within the timeout period.");
}

int main() {
	// MPC is initialized here!
	MPC mpc;

	// Wait for Kafka to init
	std::cout << "Waiting for Kafka..." << std::endl;
	wait_for_kafka();

	// Kafka configuration
    string brokers = "kafka:9092";
    string topicName = "mpc_topic";
	string errstr;

    // Create Kafka producer
    RdKafka::Conf *producerConf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    producerConf->set("metadata.broker.list", brokers, errstr);
    RdKafka::Producer *producer = RdKafka::Producer::create(producerConf, errstr);
	 if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
    }
    delete producerConf;

	// Create Kafka topic
	RdKafka::Topic *topic = RdKafka::Topic::create(producer, "mpc_server_topic", NULL, errstr);
	if (!topic) {
		cerr << "Failed to create topic: " << errstr << endl;
		exit(1);
	}

    // Create Kafka consumer
    RdKafka::Conf *consumerConf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    consumerConf->set("metadata.broker.list", brokers, errstr);
    consumerConf->set("group.id", "mpc_group", errstr);
    RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(consumerConf, errstr);
	if (!consumer) {
        cerr << "Failed to create consumer: " << errstr << endl;
        exit(1);
    }
    delete consumerConf;
    consumer->subscribe({topicName});
	std::cout << "Subscribed to " << topicName << std::endl;

	while (true) {
		 // Consume message
        RdKafka::Message *message = consumer->consume(10);
        if (message->err() == RdKafka::ERR_NO_ERROR) {
            // Get the message payload and size
			const char* payload = static_cast<const char*>(message->payload());
			size_t len = message->len();

			// Convert payload to std::string
			std::string receivedData(payload, len);
            cout << "Received data: " << receivedData << endl;

			// Parse received data and process it using MPC
			// Assuming receivedData is a JSON string
			string s = hasData(receivedData);
			if (s != "") {
				auto j = json::parse(s);
				string event = j[0].get<string>();
				if (event == "telemetry") {
					// j[1] is the data JSON object
					vector<double> ptsx = j[1]["ptsx"];
					vector<double> ptsy = j[1]["ptsy"];
					double px = j[1]["x"];
					double py = j[1]["y"];
					double psi = j[1]["psi"];
					double v = j[1]["speed"];
					double steer_angle = j[1]["steering_angle"];  // steering angle is in the opposite direction
					double acceleration = j[1]["throttle"];

					Eigen::VectorXd ptsx_car(ptsx.size());
					Eigen::VectorXd ptsy_car(ptsy.size());
					map2car(px, py, psi, ptsx, ptsy, ptsx_car, ptsy_car);

					// compute the coefficients
					auto coeffs = polyfit(ptsx_car, ptsy_car, 3); // 3rd order line fitting

					// state in car coordniates
					Eigen::VectorXd state(6); // {x, y, psi, v, cte, epsi}

					// add latency 100ms
					double latency = 0.1;
					double Lf = 2.67;
					v *= 0.44704;                             // convert from mph to m/s
					px = 0 + v * cos(0) * latency;            // px:  px0 = 0, due to the car coordinate system
					py = 0 + v * sin(0) * latency;;           // py:  psi=0 and y is point to the left of the car
					psi = 0 - v / Lf * steer_angle * latency;   // psi:  psi0 = 0, due to the car coordinate system
					double epsi = 0 - atan(coeffs[1]) - v / Lf * steer_angle * latency;
					double cte = polyeval(coeffs, 0) - 0 + v * sin(0- atan(coeffs[1])) * latency;
					v += acceleration * latency;
					state << px, py, psi, v, cte, epsi;

					// call MPC solver
					auto vars = mpc.Solve(state, coeffs);

					double steer_value;
					double throttle_value;

					steer_value = -vars[0] / deg2rad(mpc.max_steer);
					throttle_value = vars[1];

					json msgJson;
					msgJson["steering_angle"] = steer_value;
					msgJson["throttle"] = throttle_value;

					//Display the MPC predicted trajectory
					vector<double> mpc_x_vals;
					vector<double> mpc_y_vals;

					for (size_t i=2; i < vars.size(); i=i+2) { //the first two are steer angle and throttle value
					mpc_x_vals.push_back(vars[i]);
					mpc_y_vals.push_back(vars[i+1]);
					}

					msgJson["mpc_x"] = mpc_x_vals;
					msgJson["mpc_y"] = mpc_y_vals;


					// Display the waypoints/reference line
					// points are in reference to the vehicle's coordinate system
					// the points in the simulator are connected by a Yellow line
					vector<double> next_x_vals;
					vector<double> next_y_vals;

					for(int i=1; i< ptsx_car.size(); i++){ // index staring from 1 for visualize
															// only the reference point which is in the front of the car
					next_x_vals.push_back(ptsx_car[i]);
					next_y_vals.push_back(ptsy_car[i]);
					}

					msgJson["next_x"] = next_x_vals;
					msgJson["next_y"] = next_y_vals;

					std::cout << "Sending results to TCP..." << std::endl;
					string msg = "42[\"steer\"," + msgJson.dump() + "]";
					//string msg = "42[\"steer\",{\"mpc_x\":[0.0195807588064,0.04307766937408,0.0809745799859831,0.133271489820521,0.199968396467397,0.281065296556704,0.376562189156003,0.48645907882478,0.610755972726299,0.74945287333109],\"mpc_y\":[0.0,7.5008004109666e-37,2.74039965482674e-06,1.30675609008658e-05,3.71573766821909e-05,7.94940310073169e-05,0.000139230633434963,0.000208382305357721,0.000274651112945323,0.000330147579883778],\"next_x\":[3.93940137227534,25.8285057832489,48.0012942525802,67.7201992157065,88.1741885507836],\"next_y\":[0.71166777432672,1.724392909049,3.8695011146151,6.7442717046266,10.7776571055713],\"steering_angle\":-0.0188318600151931,\"throttle\":1.0}]";
					this_thread::sleep_for(chrono::milliseconds(0));
					producer->produce(topic, RdKafka::Topic::PARTITION_UA, 
									RdKafka::Producer::RK_MSG_COPY, 
									const_cast<char*>(msg.c_str()), 
									msg.size(), 
									NULL, NULL);
					producer->flush(1000);
					std::cout << "Sent..." << std::endl;
				}
			}
		}
		// } else {
		// 	// Manual driving
		// 	std::string msg = "42[\"manual\",{}]";
		// 	producer->produce(topic, RdKafka::Topic::PARTITION_UA, 
		// 							RdKafka::Producer::RK_MSG_COPY, 
		// 							const_cast<char*>(msg.c_str()), 
		// 							msg.size(), 
		// 							NULL, NULL);
		// 	producer->flush(1000);
		// }
	}
	delete topic;
}