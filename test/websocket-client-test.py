import asyncio
import websockets
import random

# import logging
# Enable logging
# logging.basicConfig(level=logging.DEBUG)

async def test_websocket():
    uri = "ws://localhost:8081/"
    try:
        async with websockets.connect(uri, subprotocols=['echo-protocol']) as websocket:
            # Send a short message
            message = f"Hello {random.randint(111,999)}!"
            print(f"Sending: {message}")
            await websocket.send(message)

            # Wait for response
            while True:
                response = await websocket.recv()
                print(f"Received: {response}")
    except websockets.exceptions.ConnectionClosed as e:
        print(f"Connection closed with error: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")

# Run the WebSocket client
asyncio.get_event_loop().run_until_complete(test_websocket())