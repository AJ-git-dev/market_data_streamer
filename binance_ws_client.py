# binance_ws_client.py
#
# Binance.US BTCUSDT WebSocket stream with keepalive ping.
# This script connectss to Binance.US's public WebSocket API and listens for real-time
# trade data for the BTC/USDT trading pair. Sends each price update to a gRPC server.

import asyncio          # Async I/O framework. 
import websockets       # WebSocket client library (async-compatible). 
import ssl              # For configuring secure TLS (SSL) connections. 
import json             # JSON parsing library (built-in). 
import grpc             # gRPC core library for Python. 
import time             # For getting current timestamps in seconds. 

# Importing generated protobuf bindings for messages and gRPC service.
import market_data_pb2
import market_data_pb2_grpc

# This is the WebSocket URL provided by Binance.US for BTCUSDT trade updates.
BINANCE_WS_URL = "wss://stream.binance.us:9443/ws/btcusdt@trade"

# Setting up an SSL context to allow encrypted communication.
# Binance.US uses TLS encryption (wss://), so we need SSL support.
ssl_context = ssl.create_default_context()    
ssl_context.check_hostname = False          
ssl_context.verify_mode = ssl.CERT_NONE    

# This function creates a gRPC channel and sends a PriceUpdate message to the server.
async def send_to_grpc(symbol: str, price: float, timestamp: int):
    async with grpc.aio.insecure_channel("localhost:50051") as channel:
        stub = market_data_pb2_grpc.MarketDataStreamerStub(channel)

        # Constructing the PriceUpdate message as defined in the .proto file.
        update = market_data_pb2.PriceUpdate(
            symbol=symbol,
            price=price,
            timestamp=timestamp
        )

        # Invoking the SendPrice RPC (unary call).
        await stub.SendPrice(update)

# This is the main async function that connects to Binance and handles the live data stream.
async def stream_trades():

    # Secure WebSocket connection to Binance using the given URL and SSL settings.
    # `ping_interval=None` disables automatic keepalive pings so we can control it manually.
    async with websockets.connect(BINANCE_WS_URL, ssl=ssl_context, ping_interval=None) as ws:
        print("Connected to Binance BTCUSDT stream")
        async def ping():
            while True:
                try:
                    await ws.ping()             
                    await asyncio.sleep(10)       # waits 10 seconds before next ping.
                except Exception:
                    break 

        # Starting the ping loop in the background without blocking the main loop.
        asyncio.create_task(ping())

        # Listening for new stream data. 
        while True:
            try:
                msg = await ws.recv()
                data = json.loads(msg)

                # Extract the "p" field, which is the trade price as a string.
                price_str = data.get("p")

                if price_str:
                    price = float(price_str)             # Convert to float.
                    timestamp = int(time.time() * 1000)  # Current timestamp in ms.
                    symbol = "BTCUSDT"

                    print(f"Live BTCUSDT: ${price_str}")

                    # Send the price update to gRPC server.
                    await send_to_grpc(symbol, price, timestamp)

            # This exception occurs if the WebSocket server closes the connection.
            except websockets.exceptions.ConnectionClosedError as e:
                print(f"Connection closed: {e}")
                break

            # Catch all other unexpected errors. 
            except Exception as e:
                print(f"Error: {e}")

# Checking if the file is being run directly and starts the event loop. 
if __name__ == "__main__":
    asyncio.run(stream_trades()) 
