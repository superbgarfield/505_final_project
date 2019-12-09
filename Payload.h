#include <string>
#include <map>
#include <iostream>
#include <iterator>
#include <algorithm>

std::string DELIMITER = "::";
std::string EMPTY_STRING = "";

std::string SENDER = "SENDER";
std::string CLIENT = "CLIENT";
std::string SERVER = "SERVER";

std::string TYPE         = "TYPE";
std::string PUT          = "PUT";
std::string GET          = "GET";
std::string CPUT         = "CPUT";
std::string DELETE 	     = "DELETE";
std::string GOLD 		 = "GOLD" ;
std::string GET_RESPONSE = "GET_RESPONSE";
std::string WRITE_RESPONSE = "WRITE_RESPONSE";
std::string LOGS         = "LOGS";
std::string CHECKPOINT   = "CHECKPOINT";
std::string SYNC_TABLET  = "SYNC_TABLET";

std::string STATUS       = "STATUS";
std::string STATUS_OK    = "STATUS_OK";
std::string STATUS_ERR   = "STATUS_ERR";

std::string DATA      = "DATA";
std::string DATA_SIZE = "DATA_SIZE";
std::string ROW       = "ROW";
std::string COLUMN    = "COLUMN";

std::string FILE_SIZE = "FILE_SIZE"; 
std::string FILE_DATA = "FILE_DATA";
std::string PING      = "PING";
std::string ON     	  = "ON";
std::string OFF       = "OFF";
std::string EMPTY_DATA = "EMPTY_DATA"; 
std::string RAW       = "RAW";
std::string STATS     = "STATS";
std::string SIZE     = "SIZE";

std::string SEQUENCE_NUMBER = "SEQUENCE_NUMBER";   


class Payload {

private:
	/* Data structure that holds key-value massage pairs */
	std::map<std::string, std::string> metadata;

public:
	/* String that contains actual data i.e. file data, message etc */
	std::string data;

	/* Adds key-value pair to message */
	void add(std::string& key, std::string& value){
		metadata[key] = value;
	}

	/* Gets value for key in message */
	std::string get(std::string& key){
		return metadata[key]; 
	}

	/* Set data */
	void set_data(std::string& str) {
		data = str;
	}

	int data_size(){
		return std::stoi(metadata[DATA_SIZE]);
	}

	/* Serializes message key-value map to string */
	std::string serialize_metadata(){

		std::string serialized_message;

		/* Iterate through map and add key::value to serialized string */
		std::map<std::string, std::string>::iterator iterator = metadata.begin();
	 	while (iterator != metadata.end()) {

	 		/* Get key-value pair to be added to messaged */
			std::string key = iterator->first;
			std::string value = iterator->second;

			/* Add to serialized message as long its not the data field */
	 		if (key != DATA) {
	 			/* Add key-value pair separeted by DELIMITER */
		 		serialized_message += key + DELIMITER + value + DELIMITER;
	 		}

			iterator++;
		}
		return serialized_message;
	}

	/* Takes a serialized message string and writes it to class message map */
	void deserialize_metadata(std::string& serialized_message) {
		/* Iterate through serialize_message string and extract key-value pairs */
		size_t delimiter_index = 0;
		while ((delimiter_index = serialized_message.find(DELIMITER)) != std::string::npos) {
			/* Extract key */
		    std::string key = serialized_message.substr(0, delimiter_index);
		    serialized_message.erase(0, delimiter_index + DELIMITER.length());

		    /* Update delimiter index */
		    delimiter_index = serialized_message.find(DELIMITER);

		    /* Extract value */
		    std::string value = serialized_message.substr(0, delimiter_index);
		   	serialized_message.erase(0, delimiter_index + DELIMITER.length());

		   	/* Update message map */
		   	metadata[key] = value;
		}

	}


};
