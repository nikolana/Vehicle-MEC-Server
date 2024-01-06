#include <uWS/uWS.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

int createPersistentSocket(const char* ipAddress, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation error" << endl;
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ipAddress, &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection Failed" << endl;
        close(sock);
        return -1;
    }

    return sock;
}

string receiveData(int sock) {
    if (sock <= 0) {
        cerr << "Invalid socket descriptor" << endl;
        return "";
    }

    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    string receivedData(buffer);

   // close(sock); // Close the socket after reading the response
    return receivedData;
}


// Function to check if the message has data
string hasData(string s) {
    auto found_null = s.find("null");
    auto b1 = s.find_first_of('[');
    auto b2 = s.find_first_of(']');
    if (found_null != string::npos) {
        return "";
    } else if (b1 != string::npos && b2 != string::npos) {
        return s.substr(b1, b2 - b1 + 1);
    }
    return "";
}

int main() {
    uWS::Hub h;

    // Establish a continuous connection at startup
    int persistentSocket = createPersistentSocket("10.0.0.145", 2222);
    if (persistentSocket < 0) {
        cerr << "Failed to establish persistent connection." << endl;
        return -1;
    }

    string msg = "-mpc-";
    send(persistentSocket, msg.c_str(), msg.length(), 0);  // identify yourself as MPC client

    h.onMessage([&h, &persistentSocket](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
        string sdata = string(data).substr(0, length);
        //string sdata = "42[\"telemetry\",{\"ptsx\":[-32.16173,-43.49173,-61.09,-78.29172,-93.05002,-107.7717],\"ptsy\":[113.361,105.941,92.88499,78.73102,65.34102,50.57938],\"psi_unity\":4.12033,\"psi\":3.733651,\"x\":-40.62,\"y\":108.73,\"steering_angle\":0,\"throttle\":0,\"speed\":0.4380091}]";
        if (sdata.size() > 2 && sdata[0] == '4' && sdata[1] == '2') {
            string s = hasData(sdata);

            // Use persistentSocket to send data
            send(persistentSocket, sdata.c_str(), sdata.length(), 0);
            cout << "Sent to server: " << sdata << endl;

            // Receive response from the server
            string response = receiveData(persistentSocket);
            cout << "Response: " << response << endl;
            ws.send(response.data(), response.length(), uWS::OpCode::TEXT);
        } else {
            // Manual driving
            // std::string msg = "42[\"manual\",{}]";
            // ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the
    // program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                        size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1) {
        res->end(s.data(), s.length());
        } else {
        // i guess this should be done more gracefully?
        res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        cout << "Connected!!!" << endl;
    });

    h.onDisconnection([&h, &persistentSocket](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
        ws.close();
        cout << "Disconnected" << endl;
        close(persistentSocket);  // Close the persistent socket on disconnection
    });

    int port = 4567;
    if (h.listen(port)) {
        cout << "Listening to port " << port << endl;
    } else {
        cerr << "Failed to listen to port" << endl;
        return -1;
    }

    h.run();
}
