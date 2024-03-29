TARGETS = server client

all: $(TARGETS)

server: bigtable_server.cpp
	g++ $^ -lpthread -lboost_serialization -lboost_system  -g -o $@ 

client: sample_client.cpp
	g++ $^ -lpthread -lboost_serialization -lboost_system -g -o $@ 

# smtp: smtp.cc
# 	g++ $< -lpthread -g -o $@

# pop3: pop3.cc
# 	g++ $^ -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lcrypto -lpthread -g -o $@

# pack:
# 	rm -f submit-hw2.zip
# 	zip -r submit-hw2.zip *.cc README Makefile

# clean::
# 	rm -fv $(TARGETS) *~

# realclean:: clean
# 	rm -fv cis505-hw2.zip
