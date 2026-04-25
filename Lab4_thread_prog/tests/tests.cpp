#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "sanitizer.h"

TEST_CASE("graph adds edge without cycle") {
    sanitizer::LockGraph graph;

    const auto report = graph.add_edge(1, 2);

    CHECK(report.has_cycle == false);
    CHECK(graph.has_edge(1, 2));
    CHECK(graph.vertex_count() == 2);
    CHECK(graph.edge_count() == 1);
}

TEST_CASE("graph detects simple ABBA cycle") {
    sanitizer::LockGraph graph;

    const auto first = graph.add_edge(1, 2);
    CHECK(first.has_cycle == false);

    const auto second = graph.add_edge(2, 1);
    CHECK(second.has_cycle == true);
    CHECK(second.new_from == 2);
    CHECK(second.new_to == 1);
    CHECK(second.cycle.size() >= 3);
}

TEST_CASE("graph detects cycle of three mutexes") {
    sanitizer::LockGraph graph;

    CHECK(graph.add_edge(10, 20).has_cycle == false);
    CHECK(graph.add_edge(20, 30).has_cycle == false);

    const auto report = graph.add_edge(30, 10);
    CHECK(report.has_cycle == true);
    CHECK(report.new_from == 30);
    CHECK(report.new_to == 10);
    CHECK(report.cycle.size() >= 4);
}

TEST_CASE("duplicate edge does not create extra cycle") {
    sanitizer::LockGraph graph;

    CHECK(graph.add_edge(1, 2).has_cycle == false);
    CHECK(graph.add_edge(1, 2).has_cycle == false);
    CHECK(graph.edge_count() == 1);
}

TEST_CASE("format cycle returns readable string") {
    const std::vector<sanitizer::MutexId> cycle = {1, 2, 3, 1};
    const std::string text = sanitizer::format_cycle(cycle);

    CHECK(text.find("0x1") != std::string::npos);
    CHECK(text.find("0x2") != std::string::npos);
    CHECK(text.find("0x3") != std::string::npos);
}