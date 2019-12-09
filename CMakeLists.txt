cmake_minimum_required(VERSION 3.15)
project(505_Final_Merge)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(505_Final_Merge
        resources/admin.html
        resources/drive.html
        resources/home.html
        resources/login.html
        resources/mail.html
        admin.h
        bigtable_server.cpp
        BigTableClient.h
        BigTableManager.h
        Checkpoint.h
        communication.h
        config.txt
        DriveClient.h
        GetHandler.cpp
        GetHandler.h
        HTMLtoString.cpp
        HTMLtoString.h
        io.h
        json.hpp
        Mail.cpp
        Mail.h
        main.cpp
        Payload.h
        PostHandler.cpp
        PostHandler.h
        sample_client.cpp
        Server.cpp
        Server.h
        Tablet.h
        email.h
        EmailClient.h
        ThreadHandler.cpp
        ThreadHandler.h
        userHandler.h
        Writer.cpp
        Writer.h)
