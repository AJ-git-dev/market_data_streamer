# Real-Time gRPC Market Data Relay

[Watch the demo video](https://github.com/AJ-git-dev/market_data_streamer/blob/main/market_data_streamer_demo.mov)


## Overview
This system simulates an HFT-relevant, real-time market data pipeline. It captures live BTCUSDT trades from Binance’s WebSocket API, parses and streams the data to a gRPC-based C++20 server which stores, timestamps, and computes a rolling moving average over the last n price ticks. The system demonstrates low-latency communication, stateless streaming, and clean message relay architecture, in the image of a production-grade trading ingestion service.

Designed for extensibility into latency benchmarking, order book construction, and market replay simulation, this pipeline enables modular testing for tick-level quantitative analytics using structured communication across languages.

---

## Tech Stack: 
- Frontend/Ingestion: Python 3.10+, ```websockets```, ```grpcio```, ```grcpio-tools```
- Streaming Protocols: WebSocket (Binance.US real-time feed), gRPC (bi-directional RPC communication)
- Backend/Analytics: C++20 with gRPC core APIs, ```protobuf```, moving average computation with double-buffered deques and mutex-guarded multithreading
- Interprocess Communication: Protocol Buffers over gRPC
- Deployment Notes: TLS-ready architecture, built for real-time system extension, live logging, and message queue integrations

---

## Protocol Buffers Definition

Located in [proto/market_data.proto](proto/market_data.proto), the schema defines:

```proto
syntax = "proto3";
package marketdata;

import "google/protobuf/empty.proto";

service MarketDataStreamer {
  rpc SendPrice (PriceUpdate) returns (google.protobuf.Empty);
}

message PriceUpdate {
  string symbol = 1;
  double price = 2;
  int64 timestamp = 3;
}
```

---

## Build Instructions

This project is built with CMake and requires: 
- C++20-compatible compiler (e.g., AppleClang 15+ or g++)
- gRPC and Protocol Buffers (installed via Homebrew or otherwise)
- Python 3.10+ with `websockets`, `grpcio`, and `grpcio-tools`

### 1. Generate gRPC Bindings
Generate C++ bindings
```
protoc -I=proto \
  --cpp_out=proto \
  --grpc_out=proto \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  proto/market_data.proto
```

Generate Python bindings
``` bash
python3 -m grpc_tools.protoc \
  -I=proto \
  --python_out=python_grpc \
  --grpc_python_out=python_grpc \
  proto/market_data.proto
```


### 2. Build C++ Server
``` bash
rm -rf build
cmake -B build -S .
cmake --build build
```

---

## Run Instructions

### Teminal 1 – Launch the gRPC Server
``` bash 
./build/server 
```

### Teminal 2 – Start Binance WebSocket + gRPC Client
``` bash 
./build/binance_ws_client.py
```

---

## Output example
- *MA(n)* denotes the moving average of the last n ticks. 
``` javascript
[gRPC Receive] Symbol: BTCUSDT | Price: $77712.62 | MA(20): $77709.18 | Timestamp: 1744135122050
```

---

## Folder Structure
``` pgsql
market_data_streamer/
├── CMakeLists.txt
├── proto/
│   ├── market_data.proto
│   ├── market_data.pb.cc
│   ├── market_data.pb.h
│   ├── market_data.grpc.pb.cc
│   ├── market_data.grpc.pb.h
├── server/
│   └── server.cpp 
├── binance_ws_client.py
├── market_data_pb2.py
├── market_data_pb2_grpc.py
```

---

## Additional Notes
- All numeric data is double-precision and timestamped in milliseconds. 
- This pipeline is designed to extend into persistent buffers, analytics, or HFT simulation modules. 
