
#include <vector>
#include <map>
#include <functional> 
#include <fstream>
#include <sstream>
#include <fstream>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/serialization/vector.hpp>

#include "communication.h"
#include "Tablet.h"

int SUCCESS =  1;
int FAILURE = -1;

class BigTableManager {

public:

	Node current_server;
	int current_server_id;

	std::vector<Node> replicas;

	void initialize(std::string config_file_path, int number_of_primaries, int server_id) {
		current_server_id = server_id;
		int index = 0;
		std::ifstream config_file(config_file_path);
		std::string address;
		while (std::getline(config_file, address)) {
			Node node;
			node.domain  = extract_domain_name_from_address(address);
			node.port    = extract_port_number_from_address(address);
			node.domain_port = node.domain + "_" + std::to_string(node.port);
			node.address = get_address_from_domain_and_port(node.domain, node.port);
			node.server_id = index;

			/* If part of the same replica group add to replicas else set current server*/
			if (index == server_id) current_server = node;
			else if (index % number_of_primaries == server_id % number_of_primaries) replicas.push_back(node);

			index++;
		} 
	}

	Tablet get_synced_tablet() {
		Tablet tablet;

		/* Read latest checkpoint on disk */
		Checkpoint checkpoint_on_disk = read_checkpoint(current_server_id);
		printf("[S] Latest checkpoint write sequence number: %d.  \n", checkpoint_on_disk.write_sequence_number );

		/* Connect to primary */
		int socket_fd = connect_to_primary_server();
		if (socket_fd == FAILURE) {
			tablet.initialize_with_checkpoint(checkpoint_on_disk);
			return tablet;
		}

		/* Declare payload to ask for logs or checkpoint from primary */
		int local_sequence_number = 0;
		if (checkpoint_on_disk.row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ] != ""){
			local_sequence_number = stoi(checkpoint_on_disk.row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ]);
		}
		std::string local_sequence_number_str = std::to_string(local_sequence_number);
		
		Payload payload;
		payload.add(TYPE, SYNC_TABLET);
		payload.add(SEQUENCE_NUMBER, local_sequence_number_str);
		payload.add(SENDER, SERVER);   
		Payload response = sender_server_request(socket_fd, payload);

		if (response.get(TYPE) == LOGS) {
			/* Replay logs */
			printf("[S] Replaying logs from primary.\n" );

			/* Deserialzing logs */
			std::vector<Log> logs;
			std::stringstream ss;
			ss.str(response.data);
			boost::archive::text_iarchive archive(ss);
			archive >> logs;

			/* Initialize tablet with latest checkpoint */
			tablet.initialize_with_checkpoint(checkpoint_on_disk);

			/* Replay logs in order */
			for (int i = 0; i < logs.size(); i++) {
				tablet.put(logs[i].row, logs[i].column, logs[i].value);
			}
			printf("[S] Replayed logs up to sequence number %d.\n", tablet.get_write_sequence_number());

			/* Send OK to primary */
			// TODO 

		}
		else if (response.get(TYPE) == CHECKPOINT) {
			printf("[S] Copying whole tablet from primary.\n" );
			/* Return tablet */
			Checkpoint checkpoint = deserialize_checkpoint(response.data);
			checkpoint.server_index = current_server_id;

			/* Initialize checkpoint with whole tablet (note: this contains latest logs) */
			tablet.initialize_with_checkpoint(checkpoint);

			/* Send OK to primary */
			// TODO 

		}

		printf("[%d] Closing connection.\n", socket_fd);
		close(socket_fd);
		return tablet;
	}

private:

	int connect_to_primary_server() {
		/* First online server is guaranteed to be primary */
		for (int i = 0; i < replicas.size(); i++) {
			/* Tries to establish connection with all servers in order
			   making sure that it doesn't try to conenct to itself */
			if (replicas[i].server_id != current_server_id) {
				int socket_fd = connect_to_server(replicas[i]);
				if (socket_fd != FAILURE) return socket_fd;
			}
		}
		printf("\n[S] There are no live replicas. Current disk checkpoint is latest.\n");
		return FAILURE;
	}

	int connect_to_server(Node node) {
		int socket_fd = open_socket(); 
		if (connect_address_to_socket(socket_fd, node.address) < 0) return FAILURE;
		printf("\n[%d] Connected to server (%s).\n", socket_fd, node.domain_port.c_str());
		return socket_fd;
	}

	Payload sender_server_request(int socket_fd, Payload& payload) {
		Payload response;

		/* Send server payload */
	 	printf("[%d] Sending %s request to server.\n", socket_fd, payload.get(TYPE).c_str());
 		send_payload_via_socket(socket_fd, payload);

 		/* Wait for server to complete reqeust */
 		recieve_payload_from_socket(socket_fd, response);
 		printf("[%d] Recieved %s with %s.\n", socket_fd, response.get(TYPE).c_str(), response.get(STATUS).c_str());

 		return response;
	}
	
};
