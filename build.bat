@echo off

set CXX=g++
set FLAGS=-Wall -Wextra -pedantic -std=c++17

call %CXX% %FLAGS% -o strings main.cpp
