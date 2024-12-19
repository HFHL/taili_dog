cmake_minimum_required(VERSION 3.10)

add_executable(b2_sport_client  b2_sport_client.cpp)
target_link_libraries(b2_sport_client unitree_sdk2 ${OpenCV_LIBS})

add_executable(b2_stand_example  b2_stand_example.cpp)
target_link_libraries(b2_stand_example unitree_sdk2)


# Find OpenCV
find_package(OpenCV REQUIRED)

# Find Boost Libraries (for Boost.Asio)
find_package(Boost REQUIRED COMPONENTS system)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# Add executable
add_executable(rtsp_server rtsp_server.cpp)

# Link libraries
target_link_libraries(rtsp_server ${OpenCV_LIBS} Boost::system pthread)