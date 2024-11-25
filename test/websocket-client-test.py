import asyncio
from enum import Enum, auto
import websockets
import json
from dataclasses import dataclass, field
import requests
import unittest
from http import HTTPStatus

# import logging
# Enable logging
# logging.basicConfig(level=logging.DEBUG)

# set host
class Hosts(Enum):
    LOCAL_HTTP = "http://localhost:8080"
    LOCAL_WS = "ws://localhost:8081"
    HOSTED_HTTP = "https://noteworthy.howdoesthiseven.work"
    HOSTED_WS = "wss://nw-ws.howdoesthiseven.work"

HOST_HTTP = Hosts.LOCAL_HTTP.value
HOST_WS = Hosts.LOCAL_WS.value

class EventObjectType(Enum):
    ROOM = 0
    PAGE = auto()
    USER = auto()
    STROKE = auto()
    SYMBOL = auto()
    SHAPE = auto()
    TEXT = auto()

@dataclass
class SendableObject():
    room_id: str

    def addMetaInformation(self, event: dict):
        event['room_id'] = self.room_id


@dataclass
class CanvasObject(SendableObject):
    owner_id: str
    page_id: str
    object_id: str

    class CanvasObjectEventType(Enum):
        CREATE = 0
        DELETE = auto() 
        MOVE = auto()
        SCALE = auto()
        ROTATE = auto()
        APPEND = auto()
        EDIT = auto()

    def addMetaInformation(self, event: dict):
        event["owner_id"] = self.owner_id
        event["page_id"] = self.page_id
        event["object_id"] = self.object_id
        super().addMetaInformation(event)

    def createMoveEvent(self, event: dict, distance_x: float, distance_y: float):
        self.addMetaInformation(event)
        event["event_type"] = self.CanvasObjectEventType.MOVE.value
        event["distance_x"] = distance_x
        event["distance_y"] = distance_y

    def createScaleEvent(self, event: dict, scale_center_x: float,
        scale_center_y: float, scale_factor_x: float,
        scale_factor_y: float):
        self.addMetaInformation(event)
        event["event_type"] = self.CanvasObjectEventType.SCALE.value
        event["scale_center_x"] = scale_center_x
        event["scale_center_y"] = scale_center_y
        event["scale_factor_x"] = scale_factor_x
        event["scale_factor_y"] = scale_factor_y

    def createRotateEvent(self, event: dict, rotation_center_x: float,
        rotation_center_y: float, rotation_degrees: float):
        self.addMetaInformation(event)
        event["event_type"] = self.CanvasObjectEventType.ROTATE.value
        event["rotation_center_x"] = rotation_center_x
        event["rotation_center_y"] = rotation_center_y
        event["rotation_degrees"] = rotation_degrees

@dataclass
class Page(SendableObject):
    page_id: int

    class PageEventType(Enum):
        CREATE = 0
        DELETE = auto()
        INSERT = auto()

    def addMetaInformation(self, event: dict):
        event["page_id"] = self.page_id
        event['object_type'] = EventObjectType.PAGE.value
        super().addMetaInformation(event)

    def createDeleteEvent(self) -> str:
        event = {}
        event['event_type'] = self.PageEventType.DELETE.value
        self.addMetaInformation(event)
        return json.dumps(event)
    
    def createInsertPageEvent(self, previous_page_id: int) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.PageEventType.INSERT.value
        event["previous_page_id"] = previous_page_id
        return json.dumps(event)

@dataclass
class Room(SendableObject):
    owner_id: str
    password: str = field(default="")

    class RoomEventType(Enum):
        CREATE = 0
        DELETE = auto()
        CHANGE_PASSWORD = auto()
        PROMOTE_USER = auto()

    def addMetaInformation(self, event: dict):
        event['object_type'] = EventObjectType.ROOM.value
        super().addMetaInformation(event)

    def createCreateEvent(self) -> str:
        event = {}
        event['event_type'] = self.RoomEventType.CREATE.value
        self.addMetaInformation(event)
        event['owner_id'] = self.owner_id
        event['password'] = self.password
        return json.dumps(event)

    def createDeleteEvent(self) -> str:
        event = {}
        event['event_type'] = self.RoomEventType.DELETE.value
        self.addMetaInformation(event)
        return json.dumps(event)

    

    def createChangePasswordEvent(self, new_password: str) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.RoomEventType.CHANGE_PASSWORD.value
        event["new_password"] = new_password
        return json.dumps(event)

    def createPromoteUserEvent(self, username: str) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.RoomEventType.PROMOTE_USER.value
        event["username"] = username
        return json.dumps(event)

@dataclass
class User(SendableObject):
    username: str
    is_connected: bool
    is_kicked: bool

    class UserEventType(Enum):
        CREATE = 0
        DELETE = auto()
        CONNECT = auto()
        DISCONNECT = auto()
        KICK = auto()

    def addMetaInformation(self, event: dict):
        event = {}
        event["username"] = self.username
        super().addMetaInformation(event)

    def createConnectEvent(self) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.UserEventType.CONNECT.value
        return json.dumps(event)

    def createDisconnectEvent(self) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.UserEventType.DISCONNECT.value
        return json.dumps(event)

    def createKickEvent(self) -> str:
        event = {}
        self.addMetaInformation(event)
        event["event_type"] = self.UserEventType.KICK.value
        return json.dumps(event)

def createRoom(owner_id: str) -> requests.Response:
    response: requests.Response = requests.post(f"{HOST_HTTP}/v1/rooms?username={owner_id}")
    return response

def createUserConnection(username: str, room_id: str, password: str = "") -> requests.Response:
    headers = {}
    if password != "":
        headers = {
            "Authorization": f"Bearer {password}"
        }
    response: requests.Response = requests.post(
        f"{HOST_HTTP}/v1/rooms/{room_id}/users?username={username}", 
        headers = headers
    )
    return response

def getRoomState(room_id: str, password: str = "") -> requests.Response:
    headers = {}
    if password != "":
        headers = {
            "Authorization": f"Bearer {password}"
        }
    response: requests.Response = requests.get(
        f"{HOST_HTTP}/v1/rooms/{room_id}", 
        headers = headers
    )
    return response

async def sendEventsToRoom(username: str, room_id: str, events: list[str]):
    uri = f"{HOST_WS}?username={username}&room_id={room_id}"
    try:
        async with websockets.connect(uri, subprotocols=[websockets.Subprotocol("echo-protocol")]) as websocket:
            for event in events:
                await websocket.send(event)

            # Wait for response with a timeout
            response = await asyncio.wait_for(websocket.recv(), timeout=0.05)
            print("Response received:", response)
    # we want it to throw these errors
    # except websockets.exceptions.ConnectionClosed as e:
    #     print("Connection was closed unexpectedly:", e)
    except asyncio.TimeoutError:
        # this is ok
        return True
    
async def runFunctionWhileConnected(username: str, room_id: str, function):
    uri = f"{HOST_WS}?username={username}&room_id={room_id}"
    try:
        async with websockets.connect(uri, subprotocols=[websockets.Subprotocol("echo-protocol")]) as websocket:
            function()
            response = await asyncio.wait_for(websocket.recv(), timeout=0.05)
            print("Response received:", response)
    # we want it to throw these errors
    # except websockets.exceptions.ConnectionClosed as e:
    #     print("Connection was closed unexpectedly:", e)
    except asyncio.TimeoutError:
        # this is ok
        return True
        

class RoomTests(unittest.TestCase):

    def test_create(self):
        username = "david"
        response: requests.Response = createRoom(username)
        self.assertEqual(response.status_code, HTTPStatus.CREATED)

    def test_get_room(self):
        username = "david"
        response: requests.Response = createRoom(username)
        self.assertEqual(response.status_code, HTTPStatus.CREATED)
        room_id = response.text
        
        response: requests.Response = getRoomState(room_id)
        self.assertEqual(response.status_code, HTTPStatus.OK)
        room_event = response.json()
        self.assertTrue(len(room_event) == 2)
        self.assertDictEqual(
            {
                "event_type": 0,
                "object_type": 0,
                "owner_id": username,
                "password": "",
                "room_id": room_id
            }
            , room_event[0])
        self.assertDictEqual(
            {
                "is_connected": False,
                "is_kicked": False,
                "event_type": User.UserEventType.CREATE.value,
                "object_type": EventObjectType.USER.value,
                "room_id": room_id,
                "username": username
            }
            , room_event[1])
        
    def test_get_fake_room(self):
        # 0 is not a valid room character so this room will never exist
        response: requests.Response = getRoomState("00000")
        self.assertEqual(response.status_code, HTTPStatus.NOT_FOUND)

    def test_connect_to_room(self):
        username = "david"
        room_id = createRoom(username).text
        self.assertTrue(asyncio.run(sendEventsToRoom(username, room_id, [])))

    def test_connect_with_fake_user(self):
        username = "david"
        room_id = createRoom(username).text
        try:
            asyncio.run(sendEventsToRoom("fake", room_id, []))
        except websockets.exceptions.ConnectionClosedOK as e:
            if e.rcvd is not None:
                self.assertEqual(e.rcvd.reason, "user not in room")

    def test_connect_fake_room(self):
        try:
            asyncio.run(sendEventsToRoom("fake", "00000", []))
        except websockets.exceptions.ConnectionClosedOK as e:
            if e.rcvd is not None:
                self.assertEqual(e.rcvd.reason, "room doesn't exist")

    def test_creating_user(self):
        room_id = createRoom("david").text
        
        username = "user"
        response: requests.Response = createUserConnection(username, room_id)
        self.assertEqual(response.status_code, HTTPStatus.CREATED)

    def test_resuming_user(self):
        room_id = createRoom("david").text
        
        username = "user"
        createUserConnection(username, room_id)
        response: requests.Response = createUserConnection(username, room_id)
        self.assertEqual(response.status_code, HTTPStatus.OK)

    def test_connecting_with_created_user(self):
        room_id = createRoom("david").text
        
        username = "user"
        createUserConnection(username, room_id)
        self.assertTrue(asyncio.run(sendEventsToRoom(username, room_id, [])))

    def test_user_already_connected(self):
        room_id = createRoom("david").text
        
        username = "user"
        createUserConnection(username, room_id)
        def testConnected():
            response: requests.Response = createUserConnection(username, room_id)
            self.assertEqual(response.status_code, HTTPStatus.CONFLICT)

        self.assertTrue(
            asyncio.run(
                runFunctionWhileConnected(username, room_id, testConnected)
                )
            )
        
    def test_change_password(self):
        username = "david"
        room_id = createRoom(username).text
        new_password = "wow"
        asyncio.run(sendEventsToRoom(username, room_id, [
            Room(room_id, username).createChangePasswordEvent(new_password)
        ]))
        room_json = getRoomState(room_id, new_password).json()
        self.assertDictEqual({
                "event_type": 0,
                "object_type": 0,
                "owner_id": username,
                "password": new_password,
                "room_id": room_id
            },room_json[0]
        )

    def test_connect_no_password_with_password(self):
        username = "david"
        room_id = createRoom(username).text
        response: requests.Response = getRoomState(room_id, "sdfijdsfdsfjk")
        self.assertEqual(response.status_code, HTTPStatus.OK)

    def test_connect_without_password_locked(self):
        username = "david"
        room_id = createRoom(username).text
        new_password = "wow"
        asyncio.run(sendEventsToRoom(username, room_id, [
            Room(room_id, username).createChangePasswordEvent(new_password)
        ]))
        response: requests.Response = getRoomState(room_id)
        self.assertEqual(response.status_code, HTTPStatus.LOCKED)

    def test_connect_wrong_password(self):
        username = "david"
        room_id = createRoom(username).text
        new_password = "wow"
        asyncio.run(sendEventsToRoom(username, room_id, [
            Room(room_id, username).createChangePasswordEvent(new_password)
        ]))
        response: requests.Response = getRoomState(room_id, "eefasf")
        self.assertEqual(response.status_code, HTTPStatus.FORBIDDEN)

    # missing tests about password stuff

class PageTests(unittest.TestCase):

    def test_create_page(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 10)
        asyncio.run(sendEventsToRoom(username, room_id, [
            page.createInsertPageEvent(previous_page_id=0)
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page.room_id,
                "page_id": page.page_id,
                "previous_page_id": 0,
            },room_json[2]
        )

    def test_create_page_after_page(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 2)
        asyncio.run(sendEventsToRoom(username, room_id, [
            Page(room_id, 1).createInsertPageEvent(previous_page_id=0),
            page.createInsertPageEvent(previous_page_id=1)
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page.room_id,
                "page_id": page.page_id,
                "previous_page_id": 0,
            },room_json[2]
        )

    def test_create_page_at_beginning(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 20)
        asyncio.run(sendEventsToRoom(username, room_id, [
            Page(room_id, 10).createInsertPageEvent(previous_page_id=0),
            page.createInsertPageEvent(previous_page_id=0)
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page.room_id,
                "page_id": page.page_id,
                "previous_page_id": 0,
            },room_json[3]
        )

    def test_create_page_between(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 15)
        asyncio.run(sendEventsToRoom(username, room_id, [
            Page(room_id, 10).createInsertPageEvent(previous_page_id=0),
            Page(room_id, 20).createInsertPageEvent(previous_page_id=10),
            page.createInsertPageEvent(previous_page_id=10)
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page.room_id,
                "page_id": page.page_id,
                "previous_page_id": 0,
            },room_json[3]
        )

    def test_delete_page(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 1234)
        asyncio.run(sendEventsToRoom(username, room_id, [
            page.createInsertPageEvent(previous_page_id=0),
            page.createDeleteEvent(),
        ]))
        room_json = getRoomState(room_id).json()
        self.assertEqual(len(room_json), 2)

    def test_delete_page_before_page(self):
        username = "david"
        room_id = createRoom(username).text
        page = Page(room_id, 1234)
        not_deleted_page = Page(room_id, 2345)
        asyncio.run(sendEventsToRoom(username, room_id, [
            page.createInsertPageEvent(previous_page_id=0),
            not_deleted_page.createInsertPageEvent(previous_page_id=0),
            page.createDeleteEvent(),
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": not_deleted_page.room_id,
                "page_id": not_deleted_page.page_id,
                "previous_page_id": 0,
            },room_json[2]
        )

    def test_delete_page_between_page(self):
        username = "david"
        room_id = createRoom(username).text
        page1 = Page(room_id, 1)
        page2 = Page(room_id, 2)
        page3 = Page(room_id, 3)
        asyncio.run(sendEventsToRoom(username, room_id, [
            page1.createInsertPageEvent(previous_page_id=0),
            page2.createInsertPageEvent(previous_page_id=1),
            page3.createInsertPageEvent(previous_page_id=2),
            page2.createDeleteEvent(),
        ]))
        room_json = getRoomState(room_id).json()
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page3.room_id,
                "page_id": page3.page_id,
                "previous_page_id": 0,
            },room_json[2]
        )
        self.assertDictEqual({
                "event_type": Page.PageEventType.INSERT.value,
                "object_type": EventObjectType.PAGE.value,
                "room_id": page1.room_id,
                "page_id": page1.page_id,
                "previous_page_id": 0,
            },room_json[3]
        )

    

def suite():
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTests(loader.loadTestsFromTestCase(PageTests))
    suite.addTests(loader.loadTestsFromTestCase(RoomTests))
    return suite

# Running the test suite
if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())