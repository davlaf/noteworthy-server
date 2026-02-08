# Sequence Diagrams

```mermaid
sequenceDiagram
    actor User
    participant Browser
    participant NoteworthyServer
    participant NoteworthyNginx

    User->>Browser: Enter "noteworthy.howdoesthiseven.work" URL
    activate Browser
    Browser->>NoteworthyNginx: HTTP GET /
    activate NoteworthyNginx
    NoteworthyNginx->>Browser: Send WebAssembly program to Browser
    deactivate NoteworthyNginx

    loop Qt Event Loop
        Browser->>Browser: Wait for event
        alt Server Event
            
            NoteworthyServer->>Browser: Browser gets WebSocket event
            Browser->>Browser: Process event
            Browser->>User: Display result of event
        else User Event
            User->>Browser: User input
            Browser->>Browser: Process event
            Browser->>User: Display Result of Event
            Browser->>NoteworthyServer: Send event to Server over WS
        end
    end
    
    User->>Browser: User exits the page

    deactivate Browser
```

## Enter name
-	Enter name
    -	Brings you to home page
```mermaid
sequenceDiagram
    actor User
    participant WelcomePage
    participant Homepage

    activate WelcomePage
    WelcomePage->>User : Display to user
   
    loop User Typing Name
        User->>WelcomePage : User adds text to name field
        WelcomePage->>WelcomePage : Enable next button
        alt Invalid Name
            User->>WelcomePage : User clears name field/enters invalid name
            WelcomePage->>WelcomePage : Disable next button
        end
    end
    User->>WelcomePage : User presses next button
    WelcomePage->>Homepage : Set Homepage username field
    WelcomePage->>Homepage : Hide self and tell Homepage to show
    deactivate WelcomePage
    activate Homepage
    Homepage->>User : Display to user
    deactivate Homepage
    
    participant WelcomePage
```
## Change name
-	Brings you back to welcome page
```mermaid
sequenceDiagram
    actor User
    participant WelcomePage
    participant Homepage

    activate Homepage
    Homepage->>User : Display to user
    User->>Homepage : User clicks change name button
    Homepage->>WelcomePage : Hide self and tell Homepage to show
    deactivate Homepage
    activate WelcomePage
    WelcomePage->>User : Display to user
    loop User Typing Name
        User->>WelcomePage : User adds text to name field
        WelcomePage->>WelcomePage : Enable next button
        alt Invalid Name
            User->>WelcomePage : User clears name field/enters invalid name
            WelcomePage->>WelcomePage : Disable next button
        end
    end
    User->>WelcomePage : User presses next button
    WelcomePage->>Homepage : Set Homepage username field
    WelcomePage->>Homepage : Hide self and tell Homepage to show
    deactivate WelcomePage
    activate Homepage
    Homepage->>User : Display to user
    deactivate Homepage
    
    participant WelcomePage
```
## Create New Room
-	Create New Room
    -	Brings you to Room page
    -	Password dialog
    -	Makes you owner
```mermaid
sequenceDiagram
    actor User
    participant Homepage
    participant RoomPage
    participant ClientWebSocketHandler
    participant NoteworthyServer
    participant RoomState

    activate Homepage
    Homepage->>User : Display to user
    User->>Homepage : User presses create room button
    Homepage->>RoomPage : Ask to create a room owned by User
    RoomPage->>ClientWebSocketHandler: Ask to connect to server
    ClientWebSocketHandler->>NoteworthyServer: Connect over WebSocket
    RoomPage->>RoomState: Ask RoomState to create a CreateRoomEvent
    RoomState->>RoomPage: Return json for CreateRoomEvent
    RoomPage->>ClientWebSocketHandler: Ask to send create room event
    ClientWebSocketHandler->>NoteworthyServer: Send create room event
    NoteworthyServer->>ClientWebSocketHandler: Send new room information
    ClientWebSocketHandler->>RoomState: Initialize room with information
    Homepage->>RoomPage : Hide self and tell RoomPage to show
    deactivate Homepage
    activate RoomPage
    RoomPage->>User: Display RoomPage telling user to create a page
    
    deactivate RoomPage
```
-	Import Room from Noteworthy File
    -	Imports state of a previously exported room
    -	Brings you to Room Editor page
    -	Makes you owner
```mermaid
sequenceDiagram
    actor User
    participant Homepage
    participant QFileDialog
    participant RoomPage
    participant ClientWebSocketHandler
    participant NoteworthyServer
    participant RoomState

    activate Homepage
    Homepage->>User : Display to User
    User->>Homepage : Presses import room button
    Homepage->>QFileDialog : <<created>>
    QFileDialog->>User: Display to user
    activate QFileDialog
    QFileDialog->>User: Ask user to locate a .ntwy file
    User->>QFileDialog: Locates .ntwy file
    QFileDialog->>Homepage: Send .ntwy filepath
    QFileDialog->>Homepage: <<destroy>>
    deactivate QFileDialog
    Homepage->>RoomPage: Ask to create room from contents of file json
    RoomPage->>RoomPage: Json parse .ntwy file
    opt Parse invalid
        RoomPage->>Homepage: Runtime parse error 
        Homepage->>User: Display error dialog
        note over Homepage: End sequence
    end
    
    RoomPage->>RoomState: call applyCreateRoomEvent with parsed json
    opt Invalid room json
        RoomState->>Homepage: Runtime exception while applying event
        Homepage->>User: Display error dialog
        note over Homepage: End sequence
    end
    RoomPage->>ClientWebSocketHandler: Ask to connect to server
    ClientWebSocketHandler->>NoteworthyServer: Connect over WebSocket
    RoomPage->>ClientWebSocketHandler: Ask to send create room event
    ClientWebSocketHandler->>NoteworthyServer: Send create room event
    NoteworthyServer->>ClientWebSocketHandler: Send new room information
    ClientWebSocketHandler->>RoomState: Add room information to existing room
    Homepage->>RoomPage : Hide self and tell RoomPage to show
    deactivate Homepage
    activate RoomPage
    RoomPage->>User: Display first page of room
    deactivate RoomPage
```
-	Join Room
    -	Brings you to Room Editor page
```mermaid
sequenceDiagram
    actor User
    participant Homepage
    participant RoomPage
    participant ClientWebSocketHandler
    participant NoteworthyServer
    participant RoomState

    activate Homepage
    Homepage->>User : Display to user
    User->>Homepage : User types in room id
    User->>Homepage : User presses join room button
    Homepage->>RoomPage : Ask to join a room with specified room id
    RoomPage->>ClientWebSocketHandler: Ask to connect to server
    ClientWebSocketHandler->>NoteworthyServer: Connect over WebSocket
    RoomPage->>ClientWebSocketHandler: Ask to fetch specified room state
    ClientWebSocketHandler->>NoteworthyServer: Fetch specified room state
    opt Room doesn't exist
        NoteworthyServer->>ClientWebSocketHandler: Send message saying room doesn't exist
        ClientWebSocketHandler->>Homepage: Raise runtime error 
        Homepage->>User: Display error dialog
        note over Homepage: End sequence
    end

    NoteworthyServer->>ClientWebSocketHandler: Send specified room state
    ClientWebSocketHandler->>RoomState: Initialize room with information
    Homepage->>RoomPage : Hide self and tell RoomPage to show
    deactivate Homepage
    activate RoomPage
    RoomPage->>User: Display RoomPage telling user to create a page
    
    deactivate RoomPage

```
## User does Action
### User who did the action
```mermaid
sequenceDiagram
    actor User
    participant RoomPage
    participant RoomState
    participant ClientWebSocketHandler
    participant NoteworthyServer
    participant ServerState

    activate RoomPage

    User->>RoomPage: User does <action>
    RoomPage->>RoomState: Ask to create a <action>Event<br>including which page and<br>room it came from
    RoomState->>RoomPage: Return created <action>Event
    RoomPage->>ClientWebSocketHandler: Simulate someone sending apply<action>Event
    ClientWebSocketHandler->>RoomState: Update state
    ClientWebSocketHandler->>RoomPage: Apply change to canvas state
    RoomPage->>User: Display visual change
    RoomPage->>ClientWebSocketHandler: Ask to send <action>Event
    ClientWebSocketHandler->>NoteworthyServer: Send <action>Event
    NoteworthyServer->>ServerState: apply<action>Event<br>in specified page and room
    NoteworthyServer->>NoteworthyServer: Send <action>Event to all other users in room
    deactivate RoomPage

```
### Other users in room
```mermaid
sequenceDiagram
    participant NoteworthyServer
    participant ClientWebSocketHandler
    participant RoomState
    participant RoomPage
    actor OtherUser
    activate RoomPage

    NoteworthyServer->>ClientWebSocketHandler: Send <action>Event
    ClientWebSocketHandler->>RoomState: Run apply<action>Event
    ClientWebSocketHandler->>RoomPage: Apply change to canvas state
    RoomPage->>OtherUser: Display visual change
    deactivate RoomPage

```
## Receive kick event
```mermaid
sequenceDiagram
    participant NoteworthyServer
    participant ClientWebSocketHandler
    participant RoomState
    participant RoomPage
    participant Homepage
    actor KickedUser

    activate RoomPage
    NoteworthyServer->>ClientWebSocketHandler: Send kickUserEvent
    ClientWebSocketHandler->>RoomState: Run applyKickUserEvent
    ClientWebSocketHandler->>RoomPage: Run userKicked function
    RoomPage->>KickedUser: Display dialog to user saying they were kicked
    RoomPage->>Homepage: Hide self and tell<br>Homepage to show
    deactivate RoomPage
    activate Homepage
    Homepage->>KickedUser: Display homepage
    deactivate Homepage
    
```
## Change Room Owner
```mermaid
sequenceDiagram
    actor RoomOwner
    participant RoomPage
    participant ClientWebSocketHandler
    participant NoteworthyServer
    participant ServerState
    participant RoomState
    actor NewOwner

    activate RoomPage
    RoomOwner->>RoomPage: Selects user to make new room owner
    RoomPage->>RoomState: Create changeRoomOwnerEvent with new owner's ID
    RoomState->>RoomPage: Return changeRoomOwnerEvent
    RoomPage->>ClientWebSocketHandler: Send changeRoomOwnerEvent
    ClientWebSocketHandler->>NoteworthyServer: Send changeRoomOwnerEvent
    NoteworthyServer->>ServerState: Apply changeRoomOwnerEvent<br>to update room ownership
    NoteworthyServer->>NoteworthyServer: Notify all clients of ownership change
    NoteworthyServer->>ClientWebSocketHandler: Send changeRoomOwnerEvent to all users
    ClientWebSocketHandler->>RoomState: Apply changeRoomOwnerEvent to local state
    RoomPage->>RoomOwner: Display ownership transfer success message
    RoomPage->>NewOwner: Notify of new ownership
    deactivate RoomPage

```
## Export Room as PDF
```mermaid
sequenceDiagram
    actor User
    participant RoomPage
    participant PDFGenerator
    participant QFileDialog

    activate RoomPage
    User->>RoomPage: Selects "Export as PDF"
    RoomPage->>PDFGenerator: Request to generate PDF from current room state
    PDFGenerator->>RoomPage: Return generated PDF file data
    RoomPage->>QFileDialog: Open dialog to save PDF file
    activate QFileDialog
    QFileDialog->>User: Prompt user to select save location
    User->>QFileDialog: Selects save location
    QFileDialog->>RoomPage: Return file path
    QFileDialog->>RoomPage: <<destroy>>
    deactivate QFileDialog
    RoomPage->>RoomPage: Save PDF data to specified file path
    RoomPage->>User: Notify user of successful PDF export
    deactivate RoomPage
```
## Export Room as Noteworthy File
```mermaid
sequenceDiagram
    actor User
    participant RoomPage
    participant RoomState
    participant QFileDialog

    activate RoomPage
    User->>RoomPage: Selects "Export as Noteworthy File"
    RoomPage->>RoomState: Request JSON for createRoomEvent
    RoomState->>RoomPage: Return JSON of createRoomEvent
    RoomPage->>QFileDialog: Open dialog to save .ntwy file
    activate QFileDialog
    QFileDialog->>User: Prompt user to select save location
    User->>QFileDialog: Selects save location
    QFileDialog->>RoomPage: Return file path
    QFileDialog->>RoomPage: <<destroy>>
    deactivate QFileDialog
    RoomPage->>RoomPage: Save JSON data to specified file path with .ntwy extension
    RoomPage->>User: Notify user of successful export
    deactivate RoomPage
```
-	
-	Export Room as Noteworthy File
