#include <iostream>
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h> 
#include <vector>
#include <string>
#include <set>
   
#include "BigTableManager.h"
  
bool PRINT_LOGS = true;
bool ACCEPT_CLIENTS = true;

int NUMBER_OF_PRIMARIES = 1;

int SERVER_INDEX;
int SOCKET;

std::set<int> open_connections; /* Set of all open TCP socket connections */ 

BigTableManager manager; 
Tablet tablet;    
 

/* Tablet wrappers */
void execute_get(int socket_fd, Payload& payload);
void execute_put_or_delete(int socket_fd, Payload& payload);
void execute_cput(int socket_fd, Payload& payload);
void execute_sync_tablet(int socket_fd, Payload& payload);
void execute_stats(int socket_fd, Payload& payload);
void execute_raw(int socket_fd, Payload& payload);
void execute_on_or_off(int socket_fd, Payload& payload);

void *client_worker(void *arg) {
	/* Parse arguments */      
	int socket_fd = *(int *)arg;   

    /* Begin receiving messages from client */  
	Payload payload;
	while (recieve_payload_from_socket(socket_fd, payload)) {
		if (PRINT_LOGS) printf("[%d] New message with type %s from %s.\n", 
			socket_fd, payload.get(TYPE).c_str(),  payload.get(SENDER).c_str());
		
		/* Execute payload */
		if      (payload.get(TYPE) == PUT)    execute_put_or_delete(socket_fd, payload);
		else if (payload.get(TYPE) == DELETE) execute_put_or_delete(socket_fd, payload);
		else if (payload.get(TYPE) == GET)    execute_get(socket_fd, payload);
		else if (payload.get(TYPE) == CPUT)   execute_cput(socket_fd, payload);
		else if (payload.get(TYPE) == SYNC_TABLET) execute_sync_tablet(socket_fd, payload);
		else if (payload.get(TYPE) == STATS)  execute_stats(socket_fd, payload);
		else if (payload.get(TYPE) == RAW)    execute_raw(socket_fd, payload);
		else if (payload.get(TYPE) == ON)     execute_on_or_off(socket_fd, payload);
		else if (payload.get(TYPE) == OFF)    execute_on_or_off(socket_fd, payload);
	}

	/* Clean up */
	if (PRINT_LOGS) printf("[%d] Closing connection.\n", socket_fd);
	close(socket_fd);
	open_connections.erase(socket_fd);
	pthread_exit(NULL);
} 
 
int main(int argc, char *argv[]) {   

	/* Parse command line arguments */
	std::string config_file_path = argv[1];
	int server_index = std::stoi(argv[2]);

	/* Initalize BigTable cluster Manager */
	manager.initialize(config_file_path, NUMBER_OF_PRIMARIES, server_index);
	tablet = manager.get_synced_tablet();

	/* Start server */
	SOCKET = open_socket();
	initialize_server(SOCKET, manager.current_server.port);
    if (PRINT_LOGS) printf("[S] Server initialized on port %d.\n", htons(manager.current_server.port));

	/* Accept new client connection and dispatch thread handler */
	// while (ACCEPT_CLIENTS) {
	// 	pthread_t thread;

	// 	/* Block until new connection is established */
	// 	int connection_fd = accept_connection(SOCKET);
  	// 	if (PRINT_LOGS) fprintf(stderr, "\n[%d] New connection.\n", connection_fd);

	// 	/* Add new connection file descriptor to set of open connections */
	// 	open_connections.insert(connection_fd);

	// 	/* Dispatch detached server thread */
	// 	pthread_create(&thread, NULL, client_worker, &connection_fd);
	// 	pthread_detach(thread);
	// }

// -----------------------------------------------------------
	while (true) {
		if (ACCEPT_CLIENTS) {
			// the usual
			while (ACCEPT_CLIENTS) {
				pthread_t thread;

				/* Block until new connection is established */
				int connection_fd = accept_connection(SOCKET);
  				if (PRINT_LOGS) fprintf(stderr, "\n[%d] New connection.\n", connection_fd);

				/* Add new connection file descriptor to set of open connections */
				open_connections.insert(connection_fd);

				/* Dispatch detached server thread */
				pthread_create(&thread, NULL, client_worker, &connection_fd);
				pthread_detach(thread);
			}
		}
		else {
			int socket_fd = accept_connection(SOCKET);
			if (PRINT_LOGS) fprintf(stderr, "\n[%d] New connection in OFF state.\n", socket_fd);

			/* Begin receiving messages from client */  
			Payload payload;
			while (recieve_payload_from_socket(socket_fd, payload)) {
				if (PRINT_LOGS) printf("[%d] New message with type %s from %s.\n", 
					socket_fd, payload.get(TYPE).c_str(),  payload.get(SENDER).c_str());
		
				/* Execute payload */
				if (payload.get(TYPE) == ON)    execute_on_or_off(socket_fd, payload);
				else {
					// TODO: send -1; change the logic on the client side
				}
			}

			/* Clean up */
			if (PRINT_LOGS) printf("[%d] Closing connection.\n", socket_fd);
			close(socket_fd);
		}
	}
// -----------------------------------------------------------
   
	return 0; 
}   

/* Execution implementation */
void execute_put_or_delete(int socket_fd, Payload& payload) {
	if (payload.get(SENDER) == CLIENT) { /* This means that this server is primary */

		/* Send write message to all replicas */
		std::vector<int> connections;
		for (int i = 0; i < manager.replicas.size(); i++) { 

			/* Try to establish connection, ignore the ones that cannot connect to 
			   for now, in the future try to turn them back on */
			int connection_fd = open_socket();
			if (connect_address_to_socket(connection_fd, manager.replicas.at(i).address) == 0) {
				connections.push_back(connection_fd);
				payload.add(SENDER, SERVER);
				send_payload_via_socket(connection_fd, payload);
				if (PRINT_LOGS) printf("[%d] Sent write message to replica %s on socket %d.\n", socket_fd, manager.replicas.at(i).domain_port.c_str(), connection_fd);
			}
			else {
				if (PRINT_LOGS) printf("[%d] Could not establish connection with replica %s.\n", socket_fd, manager.replicas.at(i).domain_port.c_str());
			}
		}
 
		/* Wait for all servers to respond */
		for (int i = 0; i < connections.size(); i++) {
			Payload response;
			recieve_payload_from_socket(connections[i], response);
			/* Handle failure response? */ // bool ok_status = (response.get(STATUS) == STATUS_OK)
			close(connections[i]);
			if (PRINT_LOGS) printf("[%d] Recieved write OK from replica on socket %d.\n", socket_fd, connections[i]);
		}

		/* Now actually do own writing */
		std::string row = payload.get(ROW);
		std::string col = payload.get(COLUMN);
		if (payload.get(TYPE) == PUT)    tablet.put(row, col, payload.data);
		if (payload.get(TYPE) == DELETE) tablet.put(row, col, EMPTY_STRING);

		/* Respond to sender saying that we write is complete */
		Payload response;
		response.add(TYPE, WRITE_RESPONSE);
		response.add(STATUS, STATUS_OK);
		send_payload_via_socket(socket_fd, response);

	}
	else if (payload.get(SENDER) == SERVER) {
		if (PRINT_LOGS) printf("[%d] Recieved write request from primary.\n", socket_fd);

		/* Write */
		std::string row = payload.get(ROW);
		std::string col = payload.get(COLUMN);
		if (payload.get(TYPE) == PUT)    tablet.put(row, col, payload.data);
		if (payload.get(TYPE) == DELETE) tablet.put(row, col, EMPTY_STRING);

		/* Send back OK */
		Payload response;
		response.add(TYPE, WRITE_RESPONSE);
		response.add(STATUS, STATUS_OK);
		send_payload_via_socket(socket_fd, response);
		if (PRINT_LOGS) printf("[%d] Sent write OK to primary.\n", socket_fd);
	}
}

void execute_get(int socket_fd, Payload& payload) {
	/* Get read parameters */ 
	std::string row    = payload.get(ROW);
	std::string column = payload.get(COLUMN); 

	/* Read from tablet */
	std::string data = tablet.get(row, column);

	/* Write back to sender */
	Payload response;
	response.add(TYPE, GET_RESPONSE);
	if (data.length() > 0) response.add(STATUS, STATUS_OK);
	else response.add(STATUS, STATUS_ERR);
	response.set_data(data);
 
	send_payload_via_socket(socket_fd, response);
}

void execute_sync_tablet(int socket_fd, Payload& payload) {
	/* Get parameters */
	int sender_checkpoint_write_seq_number = std::stoi(payload.get(SEQUENCE_NUMBER));

	/* Lock tablet */ 
	tablet.lock_tablet();    

	int local_checkpoint_write_seq_number = tablet.get_last_checkpoint_sequence_number();
	if (sender_checkpoint_write_seq_number == local_checkpoint_write_seq_number) {
		/* Send just logs */
		std::vector<Log> logs = tablet.get_logs();
 
		/* Serialize logs*/
		std::stringstream ss;
		boost::archive::text_oarchive archive(ss);
		archive << logs;
		std::string serialized_logs = ss.str();

		/* Write back to sender */ 
		Payload response;
		response.add(TYPE, LOGS);
		response.add(STATUS, STATUS_OK);
		response.set_data(serialized_logs);
		send_payload_via_socket(socket_fd, response);

		if (PRINT_LOGS) printf("[%d] Sent logs to node.\n", socket_fd);

	}
	else {
		/* Send whole tablet */
		std::string serialized_tablet = tablet.get_serialized_tablet();

		/* Write back to sender */
		Payload response;
		response.add(TYPE, CHECKPOINT);
		response.add(STATUS, STATUS_OK);
		response.set_data(serialized_tablet);
		send_payload_via_socket(socket_fd, response);

		if (PRINT_LOGS) printf("[%d] Sent whole tablet to node.\n", socket_fd);

	}

	/* Wait for OK from server? */

	/* Unlock tablet */
	tablet.unlock_tablet();

}

void execute_cput(int socket_fd, Payload& payload) {
	/* check if data in v1 is present */
	std::string row = payload.get(ROW);
	std::string col = payload.get(COLUMN);
	std::string gold  = payload.get(GOLD);
	std::string currentData = tablet.get(row, col) ;

	/* Check that the current data matches the given value */
	if (gold != currentData) {
		if (PRINT_LOGS) printf("[%d] CPUT failed due to inconsistent data.\n", socket_fd);

		/* Respond to sender: CPUT failed */
		Payload response;
		response.add(TYPE, WRITE_RESPONSE);
		response.add(STATUS, STATUS_ERR);
		send_payload_via_socket(socket_fd, response);

		return ;
	}

	if (payload.get(SENDER) == CLIENT) { /* This means that this server is primary */
		
		/* Send CPUT message to all replicas: the will only check if current value is GOLD */
		std::vector<int> connections;
		for (int i = 0; i < manager.replicas.size(); i++) {

			/* Try to establish connection, ignore the ones that cannot connect to 
			   for now, in the future try to turn them back on */
			int connection_fd = open_socket();
			if (connect_address_to_socket(connection_fd, manager.replicas.at(i).address) == 0) {
				connections.push_back(connection_fd);
				payload.add(SENDER, SERVER);
				send_payload_via_socket(connection_fd, payload);
				if (PRINT_LOGS) printf("[%d] Sent CPUT request to replica %s on socket %d.\n", socket_fd, manager.replicas.at(i).domain_port.c_str(), connection_fd);
			}
			else {
				if (PRINT_LOGS) printf("[%d] Could not establish connection with replica %s.\n", socket_fd, manager.replicas.at(i).domain_port.c_str());
			}
		}
 
		/* Wait for all servers to respond */
		bool replicasCPutSuccess = true; 
		for (int i = 0; i < connections.size(); i++) {
			Payload response;
			recieve_payload_from_socket(connections[i], response);
			if (response.get(STATUS) != STATUS_OK) { 
				replicasCPutSuccess = false ; 
			}
			//close(connections[i]);
			if (PRINT_LOGS) printf("[%d] Recieved write %s from replica on socket %d.\n", socket_fd, response.get(STATUS).c_str(), connections[i]);
		}

		Payload response;
		response.add(TYPE, WRITE_RESPONSE);

		/* insert data in replicas and your own tablet  if replicas succeeded */
		if (replicasCPutSuccess) {
			Payload putPayload = payload ;
			putPayload.add(TYPE, PUT);
			putPayload.add(SENDER, SERVER);
			if (PRINT_LOGS) printf("all replicas approved, so calling put\n");
			//execute_put_or_delete(socket_fd, putPayload) ;
			for (int i = 0; i < connections.size(); i++) {				
				send_payload_via_socket(connections[i], putPayload);
				if (PRINT_LOGS) 
					printf("[%d] Sent PUT request to replica %s on socket %d.\n", socket_fd, manager.replicas.at(i).domain_port.c_str(), connections[i]);
			}

			/* wait for them to respond */
			bool replicasPutSuccess = true; 
			for (int i = 0; i < connections.size(); i++) {
				Payload response;
				recieve_payload_from_socket(connections[i], response);
				close(connections[i]);

				if (response.get(STATUS) != STATUS_OK) { 
					replicasPutSuccess = false ; 
				}
				if (PRINT_LOGS) 
					printf("[%d] Recieved write %s from replica on socket %d.\n", socket_fd, response.get(STATUS).c_str(), connections[i]);
			}
			/* Now actually do own writing */
			std::string row = payload.get(ROW);
			std::string col = payload.get(COLUMN);
			tablet.put(row, col, payload.data);

			response.add(STATUS, STATUS_OK);
		} else {
			response.add(STATUS, STATUS_ERR);
		}

		/* Respond to sender saying that we write is complete */
		send_payload_via_socket(socket_fd, response);
	}
	else if (payload.get(SENDER) == SERVER) {
		if (PRINT_LOGS) printf("[%d] Recieved CPUT request from primary: data is present.\n", socket_fd);
		/* don't modify table yet. primary will send PUT request if all nodes agree */ 

		/* Send back OK */
		Payload response;
		response.add(TYPE, WRITE_RESPONSE);
		response.add(STATUS, STATUS_OK);
		send_payload_via_socket(socket_fd, response);
		if (PRINT_LOGS) printf("[%d] Sent CPUT-check OK to primary.\n", socket_fd);
	}
}

void execute_stats(int socket_fd, Payload& payload) {

	Payload response;
	response.add(TYPE, STATS);
	response.add(STATUS, STATUS_OK);
	size_t sz = tablet.dataSize ;
	std::string s = std::to_string(sz);
	if (PRINT_LOGS) printf("[S] Sending tablet sizeStr %s\n", s.c_str());
	response.add(SIZE, s) ;
	send_payload_via_socket(socket_fd, response);

	return ;
}

void execute_on_or_off(int socket_fd, Payload& payload) {
	Payload response;	

	if  (payload.get(TYPE) == OFF) {
		response.add(TYPE, OFF);
		if (ACCEPT_CLIENTS) {
			ACCEPT_CLIENTS = false ;
			response.add(STATUS, STATUS_OK);
			// TODO: clean the tablet, close all connections, ... what else ?
		}
		else {
			response.add(STATUS, STATUS_ERR);
		}
	}

	else {
		response.add(TYPE, ON);
		if (!ACCEPT_CLIENTS) {
			ACCEPT_CLIENTS = true ;
			response.add(STATUS, STATUS_OK);
			// TODO: actually restart everything: get checkpoints etc
		}
		else {
			response.add(STATUS, STATUS_ERR);
		}
	}

	send_payload_via_socket(socket_fd, response);

	return ;
}

void execute_raw(int socket_fd, Payload& payload) {

	/* Serialize tablet */
	std::string serializedTablet = tablet.get_serialized_tablet();

	/* Write back to sender */
	Payload response;
	response.add(TYPE, GET_RESPONSE);
	if (serializedTablet.length() > 0) response.add(STATUS, STATUS_OK);
	else response.add(STATUS, STATUS_ERR);
	response.set_data(serializedTablet);

	send_payload_via_socket(socket_fd, response);
}