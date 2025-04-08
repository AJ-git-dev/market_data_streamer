// server/server.cpp
//
// Real-Time Market Data gRPC Server.
//
// This server receives subscription requests from clients for specific trading symbols
// (e.g., BTCUSDT, ETHUSDT), and responds by streaming mock price updates over gRPC as well as moving averages.
// It simulates real-time data using a basic formula and sends updates repeatedly.
//
// This module defines and runs the gRPC service implementation of MarketDataStreamer,
// based on the Protobuf schema defined in proto/market_data.proto.

#include <iostream>
#include <string>
#include <map>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <vector>

// Access to gRPC core C++ APIs and the generated service/message classes. 
#include <grpcpp/grpcpp.h>

// This includes the generated service class and message types reated from proto/market_data.proto.
#include "market_data.grpc.pb.h"
#include "market_data.pb.h" 
#include <google/protobuf/empty.pb.h>  


using grpc::Server; // A class that represents the server. 
using grpc::ServerBuilder; // A helper to construct the server. 
using grpc::ServerContext; // Context for each RPC call. 
using grpc::ServerWriter; // Used to send a stream of messages to the client. 
using grpc::Status; // Represents the result of an RPC. 

// Accessing the .proto file's package "marketdata". 
using marketdata::MarketDataStreamer; // Service interface. 
using marketdata::PriceRequest; // Client's request message. 
using marketdata::PriceUpdate; // Server's reply message. 
using google::protobuf::Empty;


// This is the server logic class. No other class can inherit from this class. 
// This class inherits from the gRPC service. 
class MarketDataServiceImpl final : public MarketDataStreamer::Service {
public:

    // RPC Implementation. 
    std::mutex price_mutex;  // Protects shared data.
    std::vector<PriceUpdate> received_prices; // Stores received price ticks. 
    std::deque<double> price_window; // Stores recent prices for moving average calculation.
    const size_t WINDOW_SIZE = 20; 

    ::grpc::Status SendPrice(
        ::grpc::ServerContext* context, 
        const ::marketdata::PriceUpdate* request, 
        ::google::protobuf::Empty* response
    ) override {
        std::lock_guard<std::mutex> lock(price_mutex);

        // Store full tick. 
        received_prices.push_back(*request);

        // Adding new price to rolling window. 
        price_window.push_back(request->price()); 

        // Maintain window size. 
        if (price_window.size() > WINDOW_SIZE) {
            price_window.pop_front(); 
        }

        // Moving average computation. 
        double sum = 0.0; 
        for (double p : price_window) sum +=p;
        double moving_avg = price_window.empty() ? 0.0 : sum / price_window.size(); 

        std::cout << "[gRPC Receive] Symbol: " << request->symbol()
                  << " | Price: $" << request->price()
                  << " | MA(" << price_window.size() << "): $" << moving_avg
                  << " | Timestamp: " << request->timestamp() << std::endl;

        return ::grpc::Status::OK;
    }

    // Function purpose: this function overrides the one defined in the .proto file. 
    // Function parameter(s): "context" is a pointer to information about the call. 
    //                        "request" is a pointer to the client's request, given by a list of symbols. 
    //                        "writer" is a pointer to a tool that lets us stream messages back. 
    Status StreamPrices(ServerContext* context, const PriceRequest* request,
                        ServerWriter<PriceUpdate>* writer) override {

        // Returning a random integer to give the price. 
        std::srand(std::time(0));

        // Initializing a collection of key-value pairs.
        // key = symbol (e.g., "BTCUSDT"), value = its price (as a double type).
        std::map<std::string, double> symbol_prices;
        
        // This loop goes through each symbol in the request.
        // request->symbols() returns a list of strings (the symbols).
        for (int i = 0; i < request->symbols_size(); ++i) {
            std::string symbol = request->symbols(i); // access the ith symbol from the list. 
            symbol_prices[symbol] = 100.0;            // initialize each symbol at $100.00. 
        }

        // Dormant (Testing only). 
        // This infinite loop breaks only when the client disconnects.
        while (true) {

            // Checking if the client has disconnected.
            // If so, the loop is exited and the call is therefore terminated. 
            if (context->IsCancelled()) {
                break;
            }

            // Iterating through each symbol being tracked. 
            for (auto it = symbol_prices.begin(); it != symbol_prices.end(); ++it) {
                std::string symbol = it->first;
                double current_price = it->second;

                // Changing the price randomly by ± $1.00 to simulate live market conditions. 
                double change = (std::rand() % 200 - 100) / 100.0;
                double new_price = current_price + change;

                // Update is the message to be sent to the client. 
                PriceUpdate update;

                // Method calls to set fields in the message to the client. 
                update.set_symbol(symbol); // symbol string set. 
                update.set_price(new_price); // price value set. 
                update.set_timestamp(std::time(0)); // current UNIX timestamp in seconds set.  

                // Sending the message to the client.
                writer->Write(update);

                // Updating the stored price so it reflects the new price. 
                symbol_prices[symbol] = new_price;
            }
        }

        // This tells gRPC that the call completed successfully.
        return Status::OK;
    }
};

int main() {

    // This is a string that holds the address and port number where the server will run.
    // "0.0.0.0" means accept connections from any IP.
    // ":50051" is the port number arbitrarily chosen. 
    std::string server_address = "0.0.0.0:50051";

    // This server logic class object handles all client requests.
    MarketDataServiceImpl service;

    // This object is used to configure and build the server.
    grpc::ServerBuilder builder;

    // Here, we tell the builder what address the server should listen on.
    // Note that no credentials are intentionally specified here.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    // This tells the builder to register our service implementation.
    // The server will now know to handle incoming RPCs using our class.
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;

    // Ensuring to block the program and keep the server running until shut down manually. 
    server->Wait();

    return 0;
}
