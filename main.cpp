#include <iostream>
#include "Vector.h"
#include <vector>
#include <sstream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

template <typename T>
T readVectorValues(Vector<T>& vec) {
    T res{};

    for (const T& val: vec) {
        res += val;
    }

    return res;
}

TEST_CASE("Initializing vectors") {
    Vector<std::string> v;

    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == 0);

    Vector<std::string> v2(5);
    REQUIRE(v2.capacity() == 5);
    REQUIRE(v2.size() == 0);

    Vector<std::string> v3{"Some", "Strings", "In", "Here"};
    REQUIRE(v3.size() == 4);

    // Copy ctor
    Vector<std::string> v4 = v3;
    REQUIRE(readVectorValues(v3) == readVectorValues(v4));

    // Copy assignment
    Vector<std::string> v5{"One", "Two"};
    v4 = v5;

    REQUIRE(readVectorValues(v4) == readVectorValues(v5));

    Vector<std::string> v6{"String"};
    v6 = v5;

    REQUIRE(readVectorValues(v5) == readVectorValues(v6));

    // Move ctor
    Vector<std::string> v7(Vector<std::string>{"One", "two", "three"});

    // Move assignment
    Vector<std::string> v8{"Hello", "World"};
    v8 = Vector<std::string>{"One", "Two", "Three"};

    REQUIRE(readVectorValues(v8) == "OneTwoThree");

}

TEST_CASE("Capacity") {
    Vector<int> v;

    SUBCASE("Empty") {
        REQUIRE(v.empty());
    }

    SUBCASE("Size") {
        REQUIRE(v.size() == 0);

        v.push_back(1);
        v.push_back(2);
        REQUIRE(v.size() == 2);
    }

    SUBCASE("Capacity") {
        REQUIRE(v.capacity() == 0);

        v.push_back(1);
        REQUIRE(v.capacity() > 0);

        v.reserve(50);
        REQUIRE(v.capacity() >= 50);
    }

    SUBCASE("Shrink") {
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.reserve(50);
        v.shrink_to_fit();
        REQUIRE(v.capacity() == 3);
    }
}

TEST_CASE("Element access") {
    Vector<std::string> v{"Hello", "There", "Multiple", "Strings", "In", "Here"};

    SUBCASE("at") {
        REQUIRE(v.at(0) == "Hello");
        REQUIRE(v.at(3) == "Strings");
        REQUIRE_THROWS(v.at(10));

        v.at(0) = "Replaced";
        REQUIRE(v.at(0) == "Replaced");
    }

    SUBCASE("Index operator") {
        REQUIRE(v[1] == "There");
        REQUIRE(v[5] == "Here");

        v[1] = "Replaced";
        REQUIRE(v[1] == "Replaced");
    }

    SUBCASE("Front/Back") {
        REQUIRE(v.front() == "Hello");
        REQUIRE(v.back() == "Here");
    }
}

TEST_CASE("Modifiers") {
    Vector<std::string> v{"Hello", "There", "Multiple", "Strings", "In", "Here"};

    SUBCASE("Clear") {
        v.clear();
        REQUIRE(v.capacity() > 0);
        REQUIRE(v.size() == 0);
    }

    SUBCASE("Insert") {
        auto iterFrom = v.begin();
        iterFrom++;

        v.insert(iterFrom, "New");
        REQUIRE(v[1] == "New"); // rvalue

        v.reserve(50); // Prevent iterator invalidation

        iterFrom = v.begin();
        iterFrom++;

        std::string str = "Word";
        v.insert(iterFrom, str); // lvalue
        REQUIRE(v[1] == "Word");

        v.shrink_to_fit();

        v.insert(v.begin(), 3, "s");
        bool check = v[0] == "s" && v[1] == "s" && v[2] == "s";
        REQUIRE(check);

        Vector<std::string> v3{"Hello", "There", "Multiple", "Strings", "In", "Here"};
        Vector<std::string> v2{"One", "Two", "Three"};

        auto iterFrom2 = v3.begin();
        iterFrom2 += 2;

        v3.insert(iterFrom2, v2.begin(), v2.end());
    }

    SUBCASE("Erase") {
        v.erase(v.begin());
        bool check = v[0] == "There" && v[4] == "Here";
        REQUIRE(check);

        v.erase(v.begin(), v.end());
        REQUIRE(v.size() == 0);
    }

    SUBCASE("Push back") {
        v.push_back("New");
        v.push_back("Stuff");
        bool check = v[6] == "New" && v[7] == "Stuff";
        REQUIRE(check);
    }

    SUBCASE("Pop back") {
        v.pop_back();
        v.pop_back();
        bool check = v.size() == 4 && v[3] == "Strings";
        REQUIRE(check);
    }
}

TEST_CASE("Iterator") {
    Vector<std::string> v{"Hello", "There", "Multiple", "Strings", "In", "Here"};

    SUBCASE("+= / -=") {
        auto iter = v.begin();
        iter += 3;

        REQUIRE(*iter == "Strings");
    }
}

TEST_CASE("Ranged loops") {
    Vector<std::string> v{"Hello", "There", "Multiple", "Strings", "In", "Here"};
    std::stringstream ss;

    for (std::string& s: v) {
        ss << s;
    }

    const Vector<std::string> cv{"Hello", "There", "Multiple", "Strings", "In", "Here"};

    std::stringstream  ss2;

    for (const std::string& s : cv) {
        ss2 << s;
    }

    REQUIRE(ss.str() == ss2.str());
}