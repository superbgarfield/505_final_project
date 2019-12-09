#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>  
     
// #include "BigTableClient.h"
// #include "io.h" 
// #include "admin.h" 
#include "userHandler.h" 

BigTableClient big_table;

std::string CONFIG_PATH = "./config.txt";
int NUMBER_OF_PRIMARIES = 1;
// bool PRINT_LOGS = true;

void test_put(std::string& row, std::string& col, std::string value, bool print_value) {
	int ret; 
	ret = big_table.put(row, col, value);
	if (print_value) std::cout << "PUT '"<< value << "' at location [" << row << "-" << col << "] (STATUS: " << ret << ")" <<  std::endl;
	if (!print_value) std::cout << "PUT file with size "<< value.length() << " at location [" << row << "-" << col << "] (STATUS: " << ret << ")" <<  std::endl;
}

void test_get(std::string& row, std::string& col, bool print_value) {
	int ret; 
	std::string get_value;
	ret = big_table.get(row, col, get_value);
	if (print_value) std::cout << "GET '"<< get_value << "' from location [" << row << "-" << col << "] (STATUS: " << ret << ")" <<  std::endl;
	if (!print_value) std::cout << "GET file with size "<< get_value.length() << " at location [" << row << "-" << col << "] (STATUS: " << ret << ")" <<  std::endl;
}

void test_cput(std::string& row, std::string& col, std::string gold, std::string newVal, bool print_value) {
	int ret; 
	ret = big_table.cput(row, col, gold, newVal);
	if (print_value) std::cout << "CPUT '"<< newVal 
		<< "' at location [" << row << "-" << col << "] (STATUS: " << ret << ")" <<  std::endl;
}


int main(int argc, char *argv[]) { 
   
 	std::string path = "./test_pic.png";
	std::string data = read_file(path);
 
	big_table.initialize(CONFIG_PATH, NUMBER_OF_PRIMARIES, !PRINT_LOGS);

	std::string test_location1 = "test1"; 
	std::string test_location2 = "test2";  
	std::string test_location3 = "test3";  
	std::string dummy_value = "dummy data";
	std::string pic = "pic"; 
  
  	int test = 1;

	test_put(test_location1, test_location1, dummy_value, PRINT_LOGS);
	test_get(test_location1, test_location1, PRINT_LOGS);

	// test CPUT
	std::string badCPUT = "badCPUT" ;
	std::string newVal = "newVal" ;
	test_cput(test_location1, test_location1, badCPUT, newVal, PRINT_LOGS);
	test_cput(test_location1, test_location1, dummy_value, newVal, PRINT_LOGS);
	test_get(test_location1, test_location1, PRINT_LOGS);

	// test admin

	std::vector<Server> servers =  get_back_nodes(big_table);
	for (int i = 0; i < servers.size() ; i++) {
		printf("server addr =  %s.\n", servers[i].address.c_str()); 
		printf("server size =  %lu.\n", servers[i].load); 
	}

	std::string servAddr = "localhost:5000" ;
	std::vector<ServerDataPoint> serverData = get_node_data(servAddr) ;
	for (int i = 0; i < serverData.size() ; i++) {
		printf("row %s column %s\n", serverData[i].row.c_str(), serverData[i].col.c_str()); 
		printf("holds data = %s\n", serverData[i].partialData.c_str()); 
	}

	// test userHandler functions
	std::string user = "user1" ;
	std::string pass = "pass1" ;
	printf("Input with bad use returned %d\n", usr_valid(big_table, user, pass)) ;

	printf("Inserting new user/pass returned %d\n", store_usr_pass(big_table, user, pass)) ;
	printf("Inserting existing user/pass returned %d\n", store_usr_pass(big_table, user, pass)) ;

	std::string cookie = "cookie1" ;
	printf("Inserting new user/cookie returned %d\n", store_usr_cookie(big_table, user, cookie)) ;
	printf("Getting user from cookie returned %s\n", get_usr_from_cookie(big_table, cookie).c_str()) ;
	printf("Deleting exising user and cookie returned %d\n", del_usr_cookie(big_table, cookie)) ;
	printf("Deleting non-exising user and cookie returned %d\n", del_usr_cookie(big_table, cookie)) ;
	printf("Getting user from non-exising cookie returned %s\n", get_usr_from_cookie(big_table, cookie).c_str()) ;



	// /* First run test clean server */
	// if (test == 0) {
	//   	test_put(test_location1, test_location1, dummy_value, PRINT_LOGS);
	//   	test_put(test_location2, test_location2, dummy_value, PRINT_LOGS);
	//   	test_put(test_location3, test_location3, dummy_value, PRINT_LOGS);

	//   	std::cout << std::endl;
	//   	sleep(10);

	//   	test_get(test_location1, test_location1, PRINT_LOGS);
	//   	test_get(test_location2, test_location2, PRINT_LOGS);
	//   	test_get(test_location3, test_location3, PRINT_LOGS);
	// }

	// /* Dead server test */
	// if (test == 1) {
	// 	std::cout << "Primary server is dead..." << std::endl;

	//   	test_get(test_location1, test_location1, PRINT_LOGS);
	//   	test_get(test_location2, test_location2, PRINT_LOGS);
	//   	test_get(test_location3, test_location3, PRINT_LOGS);

	//   	std::cout << std::endl;

	//   	test_put(test_location1, test_location1, dummy_value, PRINT_LOGS);
	//   	test_get(test_location1, test_location1, PRINT_LOGS);
	// }


}