#include <iostream>
#include <string>
#include <vector>

#include "producer_consumer.h"

using namespace std;

int main(int argc, char* argv[]) {
  bool debug = false;
  vector<string> args;

  for (int i = 1; i < argc; ++i) {
    const string arg = argv[i];
    if (arg == "--debug") {
      debug = true;
    } else {
      args.push_back(arg);
    }
  }

  if (args.size() != 2) {
    return 1;
  }

  int consumer_count = 0;
  int sleep_limit_ms = 0;

  try {
    size_t pos = 0;
    consumer_count = stoi(args[0], &pos);
    if (pos != args[0].size()) {
      return 1;
    }

    pos = 0;
    sleep_limit_ms = stoi(args[1], &pos);
    if (pos != args[1].size()) {
      return 1;
    }
  } catch (const exception&) {
    return 1;
  }

  if (consumer_count < 0 || sleep_limit_ms < 0) {
    return 1;
  }

  vector<int> values;
  int value = 0;
  while (cin >> value) {
    values.push_back(value);
  }

  cout << run_threads(values, consumer_count, sleep_limit_ms, debug)
            << endl;
  return 0;
}