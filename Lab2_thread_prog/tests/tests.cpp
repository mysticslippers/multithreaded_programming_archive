#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <vector>

#include "producer_consumer.h"

using namespace std;

TEST_CASE("single consumer sums all values") {
  const vector<int> values{1, 2, 3, 4, 5};
  CHECK(run_threads(values, 1, 0, false) == 15);
}

TEST_CASE("multiple consumers keep correct total sum") {
  const vector<int> values{10, 20, 30, 40};
  CHECK(run_threads(values, 3, 0, false) == 100);
}

TEST_CASE("empty input returns zero") {
  const vector<int> values;
  CHECK(run_threads(values, 2, 0, false) == 0);
}

TEST_CASE("negative values are summed correctly") {
  const vector<int> values{1, -2, 3, -4, 10};
  CHECK(run_threads(values, 2, 0, false) == 8);
}

TEST_CASE("sleep limit greater than zero does not break sum") {
  const vector<int> values{1, 2, 3, 4, 5, 6};
  CHECK(run_threads(values, 4, 5, false) == 21);
}

TEST_CASE("single value with many consumers") {
  const vector<int> values{42};
  CHECK(run_threads(values, 10, 0, false) == 42);
}

TEST_CASE("zero consumers returns zero") {
  const vector<int> values{1, 2, 3};
  CHECK(run_threads(values, 0, 0, false) == 0);
}