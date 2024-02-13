cmake_minimum_required(VERSION 3.20)

project(MyProject1)

set(PostgreSQL_ADDITIONAL_VERSIONS "16")

add_executable(MyProject1 main.cpp)

add_subdirectory("C:/Projects/Database/Homework_5_PostgreSQL_Cpp/libpqxx-7.8.1" libpqxx-build)

target_compile_features(MyProject1 PRIVATE cxx_std_17)

target_link_libraries(MyProject1 pqxx)