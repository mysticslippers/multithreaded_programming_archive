CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -pthread -pedantic

TARGETS = no_deadlock deadlock_abba deadlock_cycle3

all: $(TARGETS)

no_deadlock: no_deadlock.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

deadlock_abba: deadlock_abba.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

deadlock_cycle3: deadlock_cycle3.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)