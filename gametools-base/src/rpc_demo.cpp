//
// Created by Right on 25/3/12 星期三.
//

#include "rpc_demo.h"
#include <iostream>
#include <unordered_map>
#include <rpc/server.h>
#include <rpc/this_handler.h>
#include <rpc/this_session.h>
#include "nngame.h"

double divide(double a, double b) {
    if (b == 0.0) {
        rpc::this_handler().respond_error(
                std::make_tuple(1, "Division by zero"));
    }
    return a / b;
}

struct subtractor {
    double operator()(double a, double b) {
        return a - b;
    }
};

struct multiplier {
    double multiply(double a, double b) {
        return a * b;
    }
};

int test() {
    rpc::server srv(rpc::constants::DEFAULT_PORT);
    subtractor s;
    multiplier m;

    // It's possible to bind non-capturing lambdas
    srv.bind("add", [](double a, double b) { return a + b; });
    // ... arbitrary callables
    srv.bind("sub", s);
    // ... free functions
    srv.bind("div", &divide);
    // ... member functions with captured instances in lambdas
    srv.bind("mul", [&m](double a, double b) { return m.multiply(a, b); });
    srv.bind("init", []() {
        return "ok";
    });
    std::unordered_map<rpc::session_id_t, std::string> data;

    srv.bind("store_me_maybe", [&data](std::string const& value) {
        auto id = rpc::this_session().id();
        data[id] = value;
    });

    constexpr size_t thread_count = 8;
    srv.async_run(thread_count); // non-blocking call, handlers execute on one of the workers

    std::cin.ignore();

    return 0;
}
