import asyncio
from enum import Enum, auto
import websockets
import json

# import logging
# Enable logging
# logging.basicConfig(level=logging.DEBUG)

class EventsEnum(Enum):
    NEW_STROKE = 0
    MOVE_STROKE = auto()
    DELETE_STROKE = auto()

    NEW_TEXT_BOX = auto()
    SET_CURSOR_TEXT_BOX = auto()
    DELETE_CURSOR_TEXT_BOX = auto()
    EDIT_TEXT_BOX = auto()
    TRANSFORM_TEXT_BOX = auto()
    DELETE_TEXT_BOX = auto()

    NEW_SHAPE = auto()
    TRANSFORM_SHAPE = auto()
    DELETE_SHAPE = auto()

    NEW_SYMBOL = auto()
    TRANSFORM_SYMBOL = auto()
    DELETE_SYMBOL = auto()

async def test_websocket():
    uri = "ws://localhost:8081/"
    try:
        async with websockets.connect(uri, subprotocols=['echo-protocol']) as websocket:
            # Send a short message
            message = json.dumps(
                {
                    "type": EventsEnum.NEW_SYMBOL.value
                }
            )
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