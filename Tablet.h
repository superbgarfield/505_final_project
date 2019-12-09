#include <string>
#include <vector>
#include <map>
#include <semaphore.h>
#include <mutex>
#include <iostream>

#include "Checkpoint.h"

/* The number of writes in between checkpoints */
int WRITES_PER_CHECKPOINT = 3;

std::string LOG_PUT    = "LOG_PUT";
std::string LOG_DELETE = "LOG_DELETE";
std::string LOG_CPUT   = "LOG_CPUT";
std::string NO_VALUE   = "";

struct Log {
	std::string row;
	std::string column;
	std::string type;
	std::string value;
	int write_sequence_number;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version){
		ar & row;
		ar & column;
		ar & type;
		ar & value;
		ar & write_sequence_number;
	}
};

class Tablet {

	/* Main data structure to hold tablet data */
	std::map<std::string, std::map<std::string, std::string> > row_col_value;
	int write_sequence_number;
	

	/* Save in memory write operations logs */
	std::vector<Log> logs;

	/* We want to allow many readers or only 1 writer to access a tablet row */
	std::map<std::string, sem_t> row_to_rw_semaphore;
	std::map<std::string, sem_t> row_to_reader_count_mutex;
	std::map<std::string, int>   row_to_reader_count;

	/* We want to be able to lock all writers for all rows */
	sem_t tablet_lock;
	sem_t writers_count_mutex;
	int writers_count;

	int server_index;

	public:
		size_t dataSize  ; // number of bytes occupied by data stored inside the tablet

		void initialize_with_checkpoint(Checkpoint& checkpoint) {
			server_index = checkpoint.server_index;
			row_col_value = checkpoint.row_col_value;
			write_sequence_number = checkpoint.write_sequence_number;

			// update dataSize variable
			dataSize = 0 ;
			std::map<std::string, std::map<std::string, std::string> >::iterator itRow = row_col_value.begin();
			while (itRow != row_col_value.end()) {
				
				std::map<std::string, std::string> colMap = itRow->second;
				std::map<std::string, std::string> ::iterator itCol = colMap.begin();
				while (itCol != colMap.end()) {
					std::string v = itCol->second;
					// printf("[S] string value inside column map is  %s\n", v.c_str());
					dataSize = dataSize + v.length();
					itCol++;
				}
				itRow++;
			}
			
			printf("[S] Initialized tablet. (Last write sequence number: %d)\n", write_sequence_number);

		}

		void put(std::string& row, std::string& column, std::string& value) { /* Putting an empty string is equivalent to deleting */
			/* Add to writer count */
			sem_wait(&writers_count_mutex);
			if (++writers_count == 1) sem_wait(&tablet_lock);
			sem_post(&writers_count_mutex);

			/* Acquire reader/writer semaphore */
			sem_wait(&row_to_rw_semaphore[row]);

			/* Do da thang */
			
			std::string oldData = row_col_value[row][column] ;
			row_col_value[row][column] = value;
			// update dataSize var
			dataSize = dataSize + value.length() - oldData.length() ;
			// std::cout << "dataSize = " << dataSize << "\n" ;
			
			log_write(row, column, value, LOG_PUT);
			if (write_sequence_number % WRITES_PER_CHECKPOINT == 0) checkpoint();

			/* Release reader/writer semaphore */
			sem_post(&row_to_rw_semaphore[row]);

			/* Add to writer count */
			sem_wait(&writers_count_mutex);
			if (--writers_count == 0) sem_post(&tablet_lock);
			sem_post(&writers_count_mutex);
		}	

		std::string get(std::string& row, std::string& column) {
			std::string value;

			/* Safelyt increment reader count and take semamphore if first reader */
			sem_wait(&row_to_reader_count_mutex[row]);
			if (++row_to_reader_count[row] == 1) sem_wait(&row_to_rw_semaphore[row]);
			sem_post(&row_to_reader_count_mutex[row]);

			/* Read from tabelt */
			value = row_col_value[row][column];

			/* Decrement reader count and release reader/writer semaphore if no readrs left */
			sem_wait(&row_to_reader_count_mutex[row]);
			if (--row_to_reader_count[row] == 0) sem_post(&row_to_rw_semaphore[row]);
			sem_post(&row_to_reader_count_mutex[row]);

			return value;
		}

		std::vector<Log> get_logs() {
			return logs;
		}

		int get_write_sequence_number(){
			return write_sequence_number;
		}

		void lock_tablet() {
			std::cout << "[T] Locking tablet writes." << std::endl;
			sem_wait(&tablet_lock);
		}

		void unlock_tablet() {
			std::cout << "[T] Unlocking tablet writes." << std::endl;
			sem_post(&tablet_lock);
		}

		int get_last_checkpoint_sequence_number() {
			if (row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ] == "") return 0;
			return std::stoi(row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ]);
		}

		std::string get_serialized_tablet() {
			/* Serialize whole tablet */
			Checkpoint checkpoint;
			checkpoint.write_sequence_number = write_sequence_number;
			checkpoint.row_col_value = row_col_value;
			checkpoint.server_index = server_index;
			std::cout << "[T] Sending checkpoint with seq number " << checkpoint.write_sequence_number << std::endl;
			return serialize_checkpoint(checkpoint);
		}

	private:

		/* This function adds to a queue all the write commands between checkpoints */
		void log_write(std::string &row, std::string column, std::string& value, std::string& type) {
			/* Create put log */
			Log log;
			log.row    = row;
			log.column = column;
			log.value  = value;
			log.type   = type;
			log.write_sequence_number = write_sequence_number;

			/* Push on log queue */
			logs.push_back(log);

			/* Increment Tablet write sequence number */
			write_sequence_number++;
			row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ] = std::to_string(write_sequence_number);
			row_col_value[CHECKPOINT_INDEX][CHECKPOINT_INDEX] = std::to_string(server_index);

			printf("[T] WRITE completed. (Write sequence number = %d)\n", write_sequence_number);

		}

		void checkpoint() {
			/* Add a key for sequence numberso we can extract it later */
			row_col_value[CHECKPOINT_SEQ][CHECKPOINT_SEQ] = std::to_string(write_sequence_number);
			row_col_value[CHECKPOINT_INDEX][CHECKPOINT_INDEX] = std::to_string(server_index);

			/* Serialize whole tablet */
			Checkpoint checkpoint;
			checkpoint.write_sequence_number = write_sequence_number;
			checkpoint.row_col_value = row_col_value;
			checkpoint.server_index = server_index;

			write_checkpoint_to_disk(checkpoint);

			logs.clear();
		}


};

