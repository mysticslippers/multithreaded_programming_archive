#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "sanitizer.h"

using namespace std;
using namespace sanitizer;

TEST_CASE("граф добавляет ребро без образования цикла") {
  LockGraph graph;

  const auto report = graph.add_edge(1, 2);

  CHECK_FALSE(report.has_cycle);
  CHECK(graph.has_edge(1, 2));
  CHECK(graph.vertex_count() == 2);
  CHECK(graph.edge_count() == 1);
}

TEST_CASE("граф находит простой цикл ABBA") {
  LockGraph graph;

  const auto first = graph.add_edge(1, 2);
  CHECK_FALSE(first.has_cycle);

  const auto second = graph.add_edge(2, 1);
  CHECK(second.has_cycle);
  CHECK(second.new_from == 2);
  CHECK(second.new_to == 1);
  CHECK(second.cycle.size() >= 3);
}

TEST_CASE("граф находит цикл из трёх мьютексов") {
  LockGraph graph;

  CHECK_FALSE(graph.add_edge(10, 20).has_cycle);
  CHECK_FALSE(graph.add_edge(20, 30).has_cycle);

  const auto report = graph.add_edge(30, 10);
  CHECK(report.has_cycle);
  CHECK(report.new_from == 30);
  CHECK(report.new_to == 10);
  CHECK(report.cycle.size() >= 4);
}

TEST_CASE("повторное ребро не создаёт лишний цикл") {
  LockGraph graph;

  CHECK_FALSE(graph.add_edge(1, 2).has_cycle);
  CHECK_FALSE(graph.add_edge(1, 2).has_cycle);
  CHECK(graph.edge_count() == 1);
}

TEST_CASE("цикл форматируется в читаемую строку") {
  const vector<MutexId> cycle = {1, 2, 3, 1};
  const string text = format_cycle(cycle);

  CHECK(text.find("0x1") != string::npos);
  CHECK(text.find("0x2") != string::npos);
  CHECK(text.find("0x3") != string::npos);
}