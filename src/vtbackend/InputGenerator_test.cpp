/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019-2021 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <vtbackend/InputGenerator.h>

#include <crispy/escape.h>

#include <unicode/convert.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace std;
using terminal::InputGenerator;
using terminal::Modifier;
using Buffer = terminal::InputGenerator::Sequence;
using crispy::escape;

TEST_CASE("InputGenerator.consume")
{
    auto received = string {};
    auto input = InputGenerator {};
    input.generateRaw("ABCDEF"sv);
    REQUIRE(input.peek() == "ABCDEF"sv);
    input.consume(2);
    REQUIRE(input.peek() == "CDEF"sv);
    input.consume(3);
    REQUIRE(input.peek() == "F"sv);

    input.generateRaw("abcdef"sv);
    REQUIRE(input.peek() == "Fabcdef"sv);
    input.consume(7);
    REQUIRE(input.peek() == ""sv);
}

TEST_CASE("InputGenerator.Ctrl+Space", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate(L' ', Modifier::Control);
    CHECK(escape(input.peek()) == "\\x00");
}

TEST_CASE("InputGenerator.Ctrl+A", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('A', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x01));
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+D", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('D', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x04));
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+[", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('[', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x1b)); // 27
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+\\", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('\\', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x1c)); // 28
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+]", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate(']', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x1d)); // 29
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+^", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('^', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x1e)); // 30
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.Ctrl+_", "[terminal,input]")
{
    auto input = InputGenerator {};
    input.generate('_', Modifier::Control);
    auto const c0 = string(1, static_cast<char>(0x1f)); // 31
    REQUIRE(escape(input.peek()) == escape(c0));
}

TEST_CASE("InputGenerator.all(Ctrl + A..Z)", "[terminal,input]")
{
    for (char ch = 'A'; ch <= 'Z'; ++ch)
    {
        INFO(fmt::format("Testing Ctrl+{}", ch));
        auto input = InputGenerator {};
        input.generate(static_cast<char32_t>(ch), Modifier::Control);
        auto const c0 = string(1, static_cast<char>(ch - 'A' + 1));
        REQUIRE(escape(input.peek()) == escape(c0));
    }
}