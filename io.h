#include <string>
#include <vector>
#include <map>
#include <semaphore.h>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>
 
bool is_file(std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

std::string read_file(std::string& path) {
	/* Check if file exists else return "" */
	if (!is_file(path)){
		return "";
	}
	/* Open file */
	std::ifstream infile(path.c_str(), std::ifstream::binary);

	/* Get file size */
	infile.seekg(0, infile.end);
	long file_size = infile.tellg();
	infile.seekg(0);

	/* Initialize buffer to read file */
	char* buffer = new char[file_size];

	/* Read file */
	infile.read(buffer, file_size);

	/* Close file */
	infile.close();

	/* Return file as std::string 
	   Note: file_size is key! Since buffer might contain \0 */
	return std::string(buffer, file_size);
}

void write_string_to_file(std::string& string, std::string& path) {
	/* Open file */
	std::ofstream outfile(path.c_str(),std::ofstream::binary);

	/* Write file */
	outfile.write(string.c_str(), string.length());
 
 	/* Close file */
	outfile.close();
}
