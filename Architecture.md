#Server 
1. **Initialization** : `Initialize the whole basic configuration of the world`
	- Parses the yaml files and store them in a static struct where everything is, the only thread that can modify it is the world state thread, the players threads sends command objects to the world state thread by adding them to a queue, first in first out.

2. **MainFrame** : `Loop for accept(), when there is a connection create new thread where the connection of the player lives.`
	- When a player connects, create an instance of the player class with the basic values. everything is server side, the client only sends command and receives the values needed
	- Thread is a loop, the player can choose which action he wants to make.
	- Command Queue: les threads des clients ne modifient jamais directement le world state, ils reçoivent du texte et le transforment en un objet (Command(player_id, ACTION_TAKE, "sword")) et le poussent dans une queue thread-safe. La game loop dépile les commandes une par une et mets à jour le world state de manière strictement séquentielle
	- Le client n'envoie que des actions, jamais de facts, et pour chaque action le serveur check si l'user est légitime à faire cette action.

	 > **Communication thread :** This thread loop a read and write logic to listen for commands and respond directly to the commands
	 
	 > **World state updates thread :** This threads just write to the clients the informations they need to update their local copy of the world state instance with. Not every player know evrything all the time, only what they need. 

3. **Logging** : `Log everything in the terminal where the server runs`
	- Log all client connections/disconnections with timestamps an IPs
	- Log every command received with names and parameters
	- Log all server responses and error codes sent back to clients
	- Log world state changes (item movements, npc interactions, combat results).
	- Log quest progress and completion events
	- Use json to store logs on disk in real time
	- Include log levels (INFO, WARNING, ERROR)
	- Monitor unknown patterns for abuse
	- All logs include precise timestamp


#cli-client
1. **Main menu** `Show a simple menu to connect to a server`
	- Connect option: let's you choose between connecting to a local server or to an custom ip:port `Only available when not connected to a server`
	- Resume option: let's you go back to the game when u are in the menu `Only available when connected to a server`
	- Settings option: let's you change the color palette of the cli interface
	- Quit option: let's you shutdown the client gracefully

2. **Main game interface** `When connected you begin to play...`
	- left panel showing player stats
	- low panel showing the possible command palette
	- center panel showing the room u are currently in (have to find a way to make it look kinda cool and be ultra dynamic)
	- right panel for world description and npc dialogues
	
	> **Communication thread :** This thread loop a read and write logic to send commands and listen to the response of the server for each command

	> **Background updates thread :** This thread waits all the time for world instance updates. When the server sends one, the local world state instance gets updated and the TUI gets refreshed. This thread manages entirely the refreshing of the interface

