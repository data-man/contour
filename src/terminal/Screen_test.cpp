/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019-2020 Christian Parpart <christian@parpart.family>
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
#include <terminal/MockTerm.h>
#include <terminal/Screen.h>
#include <terminal/Viewport.h>
#include <terminal/primitives.h>

#include <crispy/escape.h>
#include <crispy/utils.h>

#include <range/v3/view/iota.hpp>

#include <catch2/catch.hpp>

#include <string_view>

using crispy::escape;
using crispy::Size;
using namespace terminal;
using namespace std;

namespace // {{{
{
template <typename T>
string mainPageText(Screen<T> const& screen)
{
    return screen.renderMainPageText();
}

template <typename T>
[[maybe_unused]] void logScreenTextAlways(Screen<T> const& screen, string const& headline = "")
{
    fmt::print("{}: ZI={} cursor={} HM={}..{}\n",
               headline.empty() ? "screen dump"s : headline,
               screen.grid().zero_index(),
               screen.realCursorPosition(),
               screen.margin().horizontal.from,
               screen.margin().horizontal.to);
    fmt::print("{}\n", dumpGrid(screen.grid()));
}

template <typename T>
void logScreenText(Screen<T> const& screen, string const& headline = "")
{
    if (headline.empty())
        UNSCOPED_INFO("dump:");
    else
        UNSCOPED_INFO(headline + ":");

    for (auto const line: ::ranges::views::iota(0, *screen.pageSize().lines))
        UNSCOPED_INFO(fmt::format("[{}] \"{}\"", line, screen.grid().lineText(LineOffset::cast_from(line))));
}

// class MockScreen : public MockScreenEvents,
//                    public Screen<MockScreenEvents> {
//   public:
//     [[deprecated]] explicit MockScreen(crispy::Size _size):
//         MockScreen(PageSize{LineCount(_size.height), ColumnCount(_size.width) })
//     {
//     }
//
//     explicit MockScreen(PageSize _size,
//                         LineCount _maxHistoryLineCount = {}):
//         Screen{
//             _size,
//             *this,
//             false, // log raw
//             false, // log trace
//             _maxHistoryLineCount
//         }
//     {
//         grid().setReflowOnResize(false);
//     }
//
//     std::string windowTitle;
//     void setWindowTitle(std::string_view _title) override
//     {
//         windowTitle = _title;
//     }
// };

decltype(auto) e(string const& s)
{
    return crispy::escape(s);
}
} // namespace
// }}}

// {{{ writeText
// AutoWrap disabled: text length is less then available columns in line.
TEST_CASE("writeText.bulk.A.1", "[screen]")
{
    auto term = MockTerm(PageSize { LineCount(3), ColumnCount(5) }, LineCount(2));
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CD");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCD ");
    CHECK(screen.grid().lineText(LineOffset(1)) == "     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(4) });
}

// AutoWrap disabled: text length equals available columns in line.
TEST_CASE("writeText.bulk.A.2", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) }, LineCount(2) };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CDE");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDE");
    CHECK(screen.grid().lineText(LineOffset(1)) == "     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(4) });
}

// AutoWrap disabled: text length exceeds available columns in line.
TEST_CASE("writeText.bulk.A.3", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) }, LineCount(2) };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CDEF");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDF");
    CHECK(screen.grid().lineText(LineOffset(1)) == "     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(4) });
}

// Text does not fully fill current line.
TEST_CASE("writeText.bulk.B", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) }, LineCount(2) };
    auto& screen = term.screen();
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CD");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCD ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(4) });
}

// Text spans current line exactly.
TEST_CASE("writeText.bulk.C", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) }, LineCount(2) };
    auto& screen = term.screen();
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CDE");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDE");
    CHECK(screen.grid().lineText(LineOffset(1)) == "     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(4) });
    // Now, verify AutoWrap works by writing one char more.
    screen.write("F");
    logScreenText(screen, "AutoWrap-around");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDE");
    CHECK(screen.grid().lineText(LineOffset(1)) == "F    ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(1), ColumnOffset(1) });
}

// Text spans this line and some of the next.
TEST_CASE("writeText.bulk.D", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) }, LineCount(2) };
    auto& screen = term.screen();
    screen.write("a");
    screen.write("b");
    logScreenText(screen, "initial state");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(0), ColumnOffset(2) });
    screen.writeText("CDEF");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDE");
    CHECK(screen.grid().lineText(LineOffset(1)) == "F    ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(1), ColumnOffset(1) });
}

// Text spans full main page exactly.
TEST_CASE("writeText.bulk.E", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) }, LineCount(2) };
    auto& screen = term.screen();
    screen.write("0123456789"
                 "abcdefghij"
                 "ABCDEFGHIJ");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "0123456789");
    CHECK(screen.grid().lineText(LineOffset(1)) == "abcdefghij");
    CHECK(screen.grid().lineText(LineOffset(2)) == "ABCDEFGHIJ");
    REQUIRE(screen.cursor().position == CellLocation { LineOffset(2), ColumnOffset(9) });

    // now check if AutoWrap is triggered
    screen.write("X");
    CHECK(screen.grid().lineText(LineOffset(-1)) == "0123456789");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abcdefghij");
    CHECK(screen.grid().lineText(LineOffset(1)) == "ABCDEFGHIJ");
    CHECK(screen.grid().lineText(LineOffset(2)) == "X         ");
}

// Text spans 3 lines.
TEST_CASE("writeText.bulk.F", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) }, LineCount(1) };
    auto& screen = term.screen();
    screen.write("a");
    screen.write("b");
    screen.write("CDEFGHIJ"
                 "ABcdefghij"
                 "01234");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abCDEFGHIJ");
    CHECK(screen.grid().lineText(LineOffset(1)) == "ABcdefghij");
    CHECK(screen.grid().lineText(LineOffset(2)) == "01234     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(2), ColumnOffset(5) });
}

// Text spans 4 lines with one line being scrolled up.
TEST_CASE("writeText.bulk.G", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) }, LineCount(1) };
    auto& screen = term.screen();
    screen.write("a");
    screen.write("b");
    screen.write("CDEFGHIJ"
                 "ABCDEFGHIJ"
                 "abcdefghij"
                 "01234");
    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(-1)) == "abCDEFGHIJ");
    CHECK(screen.grid().lineText(LineOffset(0)) == "ABCDEFGHIJ");
    CHECK(screen.grid().lineText(LineOffset(1)) == "abcdefghij");
    CHECK(screen.grid().lineText(LineOffset(2)) == "01234     ");
    CHECK(screen.cursor().position == CellLocation { LineOffset(2), ColumnOffset(5) });
}

// Text spans more lines than totally available.
TEST_CASE("writeText.bulk.H", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(10) }, LineCount(1) };
    auto& screen = term.screen();

    screen.write("ABCDEFGHIJ"
                 "KLMNOPQRST"
                 "abcdefghij"
                 "0123456789");

    logScreenText(screen, "final state");
    CHECK(screen.grid().lineText(LineOffset(-1)) == "KLMNOPQRST");
    CHECK(screen.grid().lineText(LineOffset(0)) == "abcdefghij");
    CHECK(screen.grid().lineText(LineOffset(1)) == "0123456789");
    CHECK(screen.cursor().position == CellLocation { LineOffset(1), ColumnOffset(9) });
}

// TODO: Test spanning writes over all history and then reusing old lines.
// Verify we do not leak any old cell attribs.

// }}}

TEST_CASE("AppendChar", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(3) }, LineCount(1) };
    auto& screen = term.screen();
    REQUIRE(screen.historyLineCount() == LineCount(0));
    REQUIRE(screen.pageSize().lines == LineCount(1));
    REQUIRE("   " == screen.grid().lineText(LineOffset(0)));

    screen.setMode(DECMode::AutoWrap, false);

    screen.write("A");
    REQUIRE("A  " == screen.grid().lineText(LineOffset(0)));

    screen.write("B");
    REQUIRE("AB " == screen.grid().lineText(LineOffset(0)));

    screen.write("C");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));

    screen.write("D");
    REQUIRE("ABD" == screen.grid().lineText(LineOffset(0)));

    logScreenText(screen, "with AutoWrap off (before switching on)");
    screen.setMode(DECMode::AutoWrap, true);

    screen.write("E"sv);
    REQUIRE("ABE" == screen.grid().lineText(LineOffset(0)));

    screen.write("F"sv);
    CHECK("F  " == screen.grid().lineText(LineOffset(0)));
    CHECK("ABE" == screen.grid().lineText(LineOffset(-1)));
}

TEST_CASE("AppendChar_CR_LF", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE("   " == screen.grid().lineText(LineOffset(0)));

    screen.setMode(DECMode::AutoWrap, false);

    screen.write("ABC");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.write("\r");
    REQUIRE("ABC\n   \n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    screen.write("\n");
    REQUIRE("ABC\n   \n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });
}

TEST_CASE("AppendChar.emoji_exclamationmark", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(5) } };
    auto& screen = term.screen();

    screen.setBackgroundColor(IndexedColor::Blue);

    screen.write(U"\u2757"); // ❗
    // screen.write(U"\uFE0F");
    CHECK(screen.at(LineOffset(0), ColumnOffset(0)).backgroundColor() == Color::Indexed(IndexedColor::Blue));
    CHECK(screen.at(LineOffset(0), ColumnOffset(0)).width() == 2);
    CHECK(screen.at(LineOffset(0), ColumnOffset(1)).backgroundColor() == Color::Indexed(IndexedColor::Blue));
    CHECK(screen.at(LineOffset(0), ColumnOffset(1)).width() == 1);

    screen.write(U"M");
    CHECK(screen.at(LineOffset(0), ColumnOffset(2)).backgroundColor() == IndexedColor::Blue);
}

TEST_CASE("AppendChar.emoji_VS15_smiley", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(4) } };
    auto& screen = term.screen();

    // print letter-like symbol copyright sign with forced emoji presentation style.
    REQUIRE(*screen.logicalCursorPosition().column == 0);
    screen.write(U"\U0001F600");
    REQUIRE(*screen.logicalCursorPosition().column == 2);
    screen.write(U"\uFE0E");
    REQUIRE(*screen.logicalCursorPosition().column
            == 2); // U+FE0E does *NOT* lower width to 1 (easier to implement)
    screen.write(U"X");
    REQUIRE(*screen.logicalCursorPosition().column == 3);
    logScreenText(screen);

    // emoji
    auto const& c1 = screen.at(LineOffset(0), ColumnOffset(0));
    CHECK(c1.codepoints() == U"\U0001F600\uFE0E");
    CHECK(c1.width() == 2);

    // unused cell
    auto const& c2 = screen.at(LineOffset(0), ColumnOffset(1));
    CHECK(c2.empty());
    CHECK(c2.width() == 1);

    // character after the emoji
    auto const& c3 = screen.at(LineOffset(0), ColumnOffset(2));
    CHECK(c3.codepoints() == U"X");
    CHECK(c3.width() == 1);

    // tail
    auto const& c4 = screen.at(LineOffset(0), ColumnOffset(3));
    CHECK(c4.codepoints().empty());
}

TEST_CASE("AppendChar.emoji_VS16_copyright_sign", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(4) } };
    auto& screen = term.screen();
    auto const& c0 = screen.at(LineOffset(0), ColumnOffset(0));
    auto const& c1 = screen.at(LineOffset(0), ColumnOffset(1));
    auto const& c2 = screen.at(LineOffset(0), ColumnOffset(2));
    auto const& c3 = screen.at(LineOffset(0), ColumnOffset(3));

    // print letter-like symbol copyright sign with forced emoji presentation style.
    REQUIRE(screen.cursor().position.column.value == 0);
    screen.write(U"\u00A9");
    REQUIRE(screen.cursor().position.column.value == 1);
    CHECK(c0.codepointCount() == 1);
    CHECK(c0.width() == 1);
    screen.write(U"\uFE0F");
    CHECK(c0.codepointCount() == 2);
    REQUIRE(screen.cursor().position.column.value == 1);
    screen.write(U"X");
    REQUIRE(screen.cursor().position.column.value == 2);

    // double-width emoji with VS16
    CHECK(c0.codepoints() == U"\u00A9\uFE0F"sv);
    CHECK(c0.width() == 1);

    // character after the emoji
    CHECK(c1.codepoints() == U"X");
    CHECK(c1.width() == 1);

    // unused cell
    CHECK(c2.empty());
    CHECK(c2.width() == 1);

    CHECK(c3.codepoints().empty());
}

TEST_CASE("AppendChar.emoji_VS16_i", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(5) } };
    auto& screen = term.screen();
    auto const& c0 = screen.at(LineOffset(0), ColumnOffset(0));

    // print letter-like symbol `i` with forced emoji presentation style.
    screen.write(U"\u2139");
    REQUIRE(screen.cursor().position.column.value == 1);
    CHECK(c0.codepoints() == U"\u2139");
    CHECK(c0.width() == 1);

    // append into last cell
    screen.write(U"\uFE0F");
    // XXX ^^^ later on U+FE0F *will* ensure width 2 if respective mode is enabled.
    REQUIRE(screen.cursor().position.column.value == 1);
    CHECK(c0.codepoints() == U"\u2139\uFE0F");
    CHECK(c0.width() == 1);

    // write into 3rd cell
    screen.write(U"X");

    // X-cell
    auto const& c1 = screen.at(LineOffset(0), ColumnOffset(1));
    CHECK(c1.codepoints() == U"X");
    CHECK(c1.width() == 1);

    // character after the emoji
    auto const& c2 = screen.at(LineOffset(0), ColumnOffset(2));
    CHECK(c2.empty());

    auto const& c3 = screen.at(LineOffset(0), ColumnOffset(3));
    CHECK(c3.empty());

    auto const& c4 = screen.at(LineOffset(0), ColumnOffset(4));
    CHECK(c4.empty());
}

TEST_CASE("AppendChar.emoji_family", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(5) } };
    auto& screen = term.screen();
    auto const& c0 = screen.at(LineOffset(0), ColumnOffset(0));

    REQUIRE(screen.logicalCursorPosition().column.value == 0);

    // print letter-like symbol `i` with forced emoji presentation style.
    screen.write(U"\U0001F468");
    CHECK(c0.codepoints() == U"\U0001F468");
    REQUIRE(screen.logicalCursorPosition().column.value == 2);
    screen.write(U"\u200D");
    CHECK(c0.codepoints() == U"\U0001F468\u200D");
    REQUIRE(screen.logicalCursorPosition().column.value == 2);
    screen.write(U"\U0001F468");
    CHECK(c0.codepoints() == U"\U0001F468\u200D\U0001F468");
    REQUIRE(screen.logicalCursorPosition().column.value == 2);
    screen.write(U"\u200D");
    CHECK(c0.codepoints() == U"\U0001F468\u200D\U0001F468\u200D");
    REQUIRE(screen.logicalCursorPosition().column.value == 2);
    screen.write(U"\U0001F467");
    CHECK(c0.codepoints() == U"\U0001F468\u200D\U0001F468\u200D\U0001F467");
    REQUIRE(screen.logicalCursorPosition().column.value == 2);
    screen.write(U"X");
    REQUIRE(screen.logicalCursorPosition().column.value == 3);

    // double-width emoji with VS16
    auto const& c1 = screen.at(LineOffset(0), ColumnOffset(0));
    CHECK(c1.codepoints() == U"\U0001F468\u200D\U0001F468\u200D\U0001F467");
    CHECK(c1.width() == 2);

    // unused cell
    auto const& c2 = screen.at(LineOffset(0), ColumnOffset(1));
    CHECK(c2.codepointCount() == 0);
    CHECK(c2.width() == 1);

    // character after the emoji
    auto const& c3 = screen.at(LineOffset(0), ColumnOffset(2));
    CHECK(c3.codepoints() == U"X");
    CHECK(c3.width() == 1);
}

TEST_CASE("AppendChar.emoji_zwj_1", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(5) } };
    auto& screen = term.screen();

    screen.setMode(DECMode::AutoWrap, false);

    // https://emojipedia.org/man-facepalming-medium-light-skin-tone/
    auto const emoji = u32string_view { U"\U0001F926\U0001F3FC\u200D\u2642\uFE0F" };
    screen.write(emoji);

    auto const& c0 = screen.at(LineOffset(0), ColumnOffset(0));
    CHECK(c0.codepoints() == emoji);
    CHECK(c0.width() == 2);

    // other columns remain untouched
    CHECK(screen.at(LineOffset(0), ColumnOffset(1)).empty());
    CHECK(screen.at(LineOffset(0), ColumnOffset(2)).empty());
    CHECK(screen.at(LineOffset(0), ColumnOffset(3)).empty());
    CHECK(screen.at(LineOffset(0), ColumnOffset(4)).empty());

    auto const s8 = screen.grid().lineText(LineOffset(0));
    auto const s32 = unicode::from_utf8(s8);
    CHECK(U"\U0001F926\U0001F3FC\u200D\u2642\uFE0F" == c0.codepoints());
    CHECK(U"\U0001F926\U0001F3FC\u200D\u2642\uFE0F    " == s32);
}

TEST_CASE("AppendChar.emoji_1", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(3) } };
    auto& screen = term.screen();

    screen.write(U"\U0001F600");

    auto const& c1 = screen.at(LineOffset(0), ColumnOffset(0));
    CHECK(c1.codepoints() == U"\U0001F600");
    CHECK(c1.width() == 2);
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    CHECK(screen.at(LineOffset(0), ColumnOffset(1)).codepointCount() == 0);
    CHECK(screen.at(LineOffset(0), ColumnOffset(2)).codepointCount() == 0);

    screen.write("B");
    auto const& c2 = screen.at(LineOffset(0), ColumnOffset(1));
    CHECK(c2.codepointCount() == 0);
    CHECK(c2.codepoints().empty());
    CHECK(c2.width() == 1);

    auto const& c3 = screen.at(LineOffset(0), ColumnOffset(2));
    CHECK(c3.codepointCount() == 1);
    CHECK(c3.codepoint(0) == 'B');
    CHECK(c3.width() == 1);
}

TEST_CASE("AppendChar_WideChar", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, true);
    screen.write(U"\U0001F600");
    CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });
}

TEST_CASE("AppendChar_AutoWrap", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, true);

    screen.write("ABC");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("   " == screen.grid().lineText(LineOffset(1)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.write("D");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("D  " == screen.grid().lineText(LineOffset(1)));

    screen.write("EF");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("DEF" == screen.grid().lineText(LineOffset(1)));

    logScreenText(screen);
    screen.write("G");
    logScreenText(screen);
    REQUIRE("DEF" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("G  " == screen.grid().lineText(LineOffset(1)));
}

TEST_CASE("AppendChar_AutoWrap_LF", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, true);

    INFO("write ABC");
    screen.write("ABC");
    logScreenText(screen);
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("   " == screen.grid().lineText(LineOffset(1)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    INFO("write CRLF");
    screen.write("\r\n");
    logScreenText(screen, "after writing LF");
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });

    INFO("write 'D'");
    screen.write("D");
    logScreenText(screen);
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("D  " == screen.grid().lineText(LineOffset(1)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
}

TEST_CASE("Screen.isLineVisible", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(2) }, LineCount(5) };
    auto& screen = term.screen();
    auto viewport = terminal::Viewport { screen };

    screen.write("10203040");
    logScreenText(screen);
    CHECK(screen.grid().lineText(LineOffset(0)) == "40");
    CHECK(screen.grid().lineText(LineOffset(-1)) == "30");
    CHECK(screen.grid().lineText(LineOffset(-2)) == "20");
    CHECK(screen.grid().lineText(LineOffset(-3)) == "10");

    CHECK(viewport.isLineVisible(LineOffset { 0 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -1 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -2 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -3 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -4 })); // minimal out-of-bounds

    viewport.scrollUp(LineCount(1));
    REQUIRE(viewport.scrollOffset() == ScrollOffset(1));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { 0 }));
    CHECK(viewport.isLineVisible(LineOffset { -1 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -2 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -3 }));

    viewport.scrollUp(LineCount(1));
    REQUIRE(viewport.scrollOffset() == ScrollOffset(2));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { 0 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -1 }));
    CHECK(viewport.isLineVisible(LineOffset { -2 }));
    CHECK_FALSE(viewport.isLineVisible(LineOffset { -3 }));

    viewport.scrollUp(LineCount(1));
    REQUIRE(viewport.scrollOffset() == ScrollOffset(3));
    CHECK(!viewport.isLineVisible(LineOffset { 0 }));
    CHECK(!viewport.isLineVisible(LineOffset { -1 }));
    CHECK(!viewport.isLineVisible(LineOffset { -2 }));
    CHECK(viewport.isLineVisible(LineOffset { -3 }));
}

TEST_CASE("Backspace", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    screen.write("12");
    CHECK("12 " == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.write("\b");
    CHECK("12 " == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    screen.write("\b");
    CHECK("12 " == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    screen.write("\b");
    CHECK("12 " == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
}

TEST_CASE("Linefeed", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();
    SECTION("with scroll-up")
    {
        INFO("init:");
        INFO(fmt::format("  line 1: '{}'", screen.grid().lineText(LineOffset(0))));
        INFO(fmt::format("  line 2: '{}'", screen.grid().lineText(LineOffset(1))));

        screen.write("1\r\n2");

        INFO("after writing '1\\n2':");
        INFO(fmt::format("  line 1: '{}'", screen.grid().lineText(LineOffset(0))));
        INFO(fmt::format("  line 2: '{}'", screen.grid().lineText(LineOffset(1))));

        REQUIRE("1 " == screen.grid().lineText(LineOffset(0)));
        REQUIRE("2 " == screen.grid().lineText(LineOffset(1)));

        screen.write("\r\n3"); // line 3

        INFO("After writing '\\n3':");
        INFO(fmt::format("  line 1: '{}'", screen.grid().lineText(LineOffset(0))));
        INFO(fmt::format("  line 2: '{}'", screen.grid().lineText(LineOffset(1))));

        REQUIRE("2 " == screen.grid().lineText(LineOffset(0)));
        REQUIRE("3 " == screen.grid().lineText(LineOffset(1)));
    }
}

TEST_CASE("ClearToEndOfScreen", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("ABC\r\nDEF\r\nGHI");

    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("DEF" == screen.grid().lineText(LineOffset(1)));
    REQUIRE("GHI" == screen.grid().lineText(LineOffset(2)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(2) });

    logScreenText(screen);
    screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 1 });
    screen.clearToEndOfScreen();
    logScreenText(screen);

    CHECK("ABC" == screen.grid().lineText(LineOffset(0)));
    CHECK("D  " == screen.grid().lineText(LineOffset(1)));
    CHECK("   " == screen.grid().lineText(LineOffset(2)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
}

TEST_CASE("ClearToBeginOfScreen", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("ABC\r\nDEF\r\nGHI");

    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("DEF" == screen.grid().lineText(LineOffset(1)));
    REQUIRE("GHI" == screen.grid().lineText(LineOffset(2)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(2) });

    screen.moveCursorTo(LineOffset(1), ColumnOffset(1));
    screen.clearToBeginOfScreen();

    CHECK("   " == screen.grid().lineText(LineOffset(0)));
    CHECK("  F" == screen.grid().lineText(LineOffset(1)));
    CHECK("GHI" == screen.grid().lineText(LineOffset(2)));
    CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
}

TEST_CASE("ClearScreen", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();
    screen.write("AB\r\nC");
    screen.clearScreen();
    CHECK("  " == screen.grid().lineText(LineOffset(0)));
    CHECK("  " == screen.grid().lineText(LineOffset(1)));
}

TEST_CASE("ClearToEndOfLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("ABC");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));

    screen.moveCursorToColumn(ColumnOffset(1));
    screen.clearToEndOfLine();
    CHECK("A  " == screen.grid().lineText(LineOffset(0)));
}

TEST_CASE("ClearToBeginOfLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.write("ABC");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));

    screen.moveCursorToColumn(ColumnOffset(1));
    screen.clearToBeginOfLine();
    CHECK("  C" == screen.grid().lineText(LineOffset(0)));
}

TEST_CASE("ClearLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(1), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.write("ABC");
    REQUIRE("ABC" == screen.grid().lineText(LineOffset(0)));

    screen.clearLine();
    CHECK("   " == screen.grid().lineText(LineOffset(0)));
}

TEST_CASE("DECFI", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.setMode(DECMode::LeftRightMargin, true);
    screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3));
    screen.setTopBottomMargin(LineOffset(1), LineOffset(3));
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    screen.write("\033[1;1H");

    // from 0,0 to 0,1 (from outside margin to left border)
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    // from 0,1 to 0,2
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    // from 0,2 to 0,3
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(3) });
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    // from 0,3 to 0,3, scrolling 1 left
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(3) });
    REQUIRE("12345\n689 0\nACD E\nFHI J\nKLMNO\n" == screen.renderMainPageText());

    // from 0,3 to 0,3, scrolling 1 left
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(3) });
    REQUIRE("12345\n69  0\nAD  E\nFI  J\nKLMNO\n" == screen.renderMainPageText());

    // from 0,3 to 0,3, scrolling 1 left (now all empty)
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(3) });
    REQUIRE("12345\n6   0\nA   E\nF   J\nKLMNO\n" == screen.renderMainPageText());

    // from 0,3 to 0,3, scrolling 1 left (looks just like before)
    screen.write("\0339");
    REQUIRE(screen.realCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(3) });
    REQUIRE("12345\n6   0\nA   E\nF   J\nKLMNO\n" == screen.renderMainPageText());
}

TEST_CASE("InsertColumns", "[screen]")
{
    // "DECIC has no effect outside the scrolling margins."
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");

    screen.setMode(DECMode::LeftRightMargin, true);
    screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3));
    screen.setTopBottomMargin(LineOffset(1), LineOffset(3));

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    SECTION("outside margins: top left")
    {
        screen.moveCursorTo({}, {});
        screen.insertColumns(ColumnCount(1));
        REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("outside margins: bottom right")
    {
        screen.moveCursorTo(LineOffset(4), ColumnOffset(4));
        screen.insertColumns(ColumnCount(1));
        REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("inside margins")
    {
        screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 2 });
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

        SECTION("DECIC-0")
        {
            screen.insertColumns(ColumnCount(0));
            REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
        }

        SECTION("DECIC-1")
        {
            screen.insertColumns(ColumnCount(1));
            REQUIRE("12345\n67 80\nAB CE\nFG HJ\nKLMNO\n" == screen.renderMainPageText());
        }

        SECTION("DECIC-2")
        {
            screen.insertColumns(ColumnCount(2));
            REQUIRE("12345\n67  0\nAB  E\nFG  J\nKLMNO\n" == screen.renderMainPageText());
        }

        SECTION("DECIC-2 (another)")
        {
            screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 1 });
            screen.insertColumns(ColumnCount(2));
            REQUIRE("12345\n6  70\nA  BE\nF  GJ\nKLMNO\n" == screen.renderMainPageText());
        }

        SECTION("DECIC-3-clamped")
        {
            screen.insertColumns(ColumnCount(3));
            REQUIRE("12345\n67  0\nAB  E\nFG  J\nKLMNO\n" == screen.renderMainPageText());
        }
    }

    SECTION("inside margins - repeative")
    {
        screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 1 });
        screen.insertColumns(ColumnCount(1));
        REQUIRE("12345\n6 780\nA BCE\nF GHJ\nKLMNO\n" == screen.renderMainPageText());
        screen.insertColumns(ColumnCount(1));
        REQUIRE("12345\n6  70\nA  BE\nF  GJ\nKLMNO\n" == screen.renderMainPageText());
    }
}

TEST_CASE("InsertCharacters.NoMargins", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("123\r\n456");
    screen.write("\033[2;2H");
    REQUIRE("123\n456\n" == screen.renderMainPageText());
    REQUIRE(screen.realCursorPosition().line == LineOffset(1));
    REQUIRE(screen.realCursorPosition().column == ColumnOffset(1));

    SECTION("default")
    {
        screen.write("\033[@");
        REQUIRE("123\n4 5\n" == screen.renderMainPageText());
    }

    SECTION("ICH: 1 like default")
    {
        screen.write("\033[1@");
        REQUIRE("123\n4 5\n" == screen.renderMainPageText());
    }

    SECTION("ICH: exact match")
    {
        screen.write("\033[2@");
        REQUIRE("123\n4  \n" == screen.renderMainPageText());
    }

    SECTION("ICH: one overflow")
    {
        screen.write("\033[3@");
        REQUIRE("123\n4  \n" == screen.renderMainPageText());
    }

    SECTION("ICH: full line (n-1)")
    {
        screen.write("\033[2;1H");
        screen.write("\033[2@");
        REQUIRE("123\n  4\n" == screen.renderMainPageText());
    }

    SECTION("ICH: full line (n)")
    {
        screen.write("\033[2;1H");
        screen.write("\033[3@");
        REQUIRE("123\n   \n" == screen.renderMainPageText());
    }

    SECTION("ICH: full line (n+1)")
    {
        screen.write("\033[2;1H");
        screen.write("\033[4@");
        REQUIRE("123\n   \n" == screen.renderMainPageText());
    }
}

TEST_CASE("InsertCharacters.Margins", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n678");
    screen.write("90");

    screen.setMode(DECMode::LeftRightMargin, true);
    screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3));
    REQUIRE("12345\n67890\n" == screen.renderMainPageText());

    SECTION("outside margins: left")
    {
        screen.moveCursorTo(LineOffset(0), ColumnOffset(0));
        screen.insertCharacters(ColumnCount(1));
        REQUIRE("12345\n67890\n" == screen.renderMainPageText());
    }

    SECTION("outside margins: right")
    {
        screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 4 });
        screen.insertCharacters(ColumnCount(1));
        REQUIRE("12345\n67890\n" == screen.renderMainPageText());
    }

    SECTION("inside margins")
    {
        screen.moveCursorTo(LineOffset(0), ColumnOffset(2));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

        SECTION("no-op")
        {
            screen.insertCharacters(ColumnCount(0));
            CHECK(screen.renderMainPageText() == "12345\n67890\n");
        }

        SECTION("ICH-1")
        {
            screen.insertCharacters(ColumnCount(1));
            CHECK(screen.renderMainPageText() == "12 35\n67890\n");
        }

        SECTION("ICH-2")
        {
            screen.insertCharacters(ColumnCount(2));
            CHECK(screen.renderMainPageText() == "12  5\n67890\n");
        }

        SECTION("ICH-3-clamped")
        {
            screen.insertCharacters(ColumnCount(3));
            REQUIRE(screen.renderMainPageText() == "12  5\n67890\n");
        }
    }
}

TEST_CASE("InsertLines", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(6), ColumnCount(4) } };
    auto& screen = term.screen();
    screen.write("1234\r\n5678\r\nABCD\r\nEFGH\r\nIJKL\r\nMNOP");
    REQUIRE("1234\n5678\nABCD\nEFGH\nIJKL\nMNOP\n" == screen.renderMainPageText());

    SECTION("old")
    {
        auto term = MockTerm { PageSize { LineCount(3), ColumnCount(2) } };
        auto& screen = term.screen();

        screen.write("AB\r\nCD");
        REQUIRE("AB" == screen.grid().lineText(LineOffset(0)));
        REQUIRE("CD" == screen.grid().lineText(LineOffset(1)));
        REQUIRE("  " == screen.grid().lineText(LineOffset(2)));

        logScreenText(screen, "A");
        screen.insertLines(LineCount(1));
        logScreenText(screen, "B");
        CHECK("AB" == screen.grid().lineText(LineOffset(0)));
        CHECK("  " == screen.grid().lineText(LineOffset(1)));
        CHECK("CD" == screen.grid().lineText(LineOffset(2)));

        screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 0 });
        screen.insertLines(LineCount(1));
        CHECK("  " == screen.grid().lineText(LineOffset(0)));
        CHECK("AB" == screen.grid().lineText(LineOffset(1)));
        CHECK("  " == screen.grid().lineText(LineOffset(2)));
    }
    // TODO: test with (top/bottom and left/right) margins enabled
}

TEST_CASE("DeleteLines", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(2) } };
    auto& screen = term.screen();

    screen.write("AB\r\nCD\r\nEF");
    logScreenText(screen, "initial");
    REQUIRE("AB" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("CD" == screen.grid().lineText(LineOffset(1)));
    REQUIRE("EF" == screen.grid().lineText(LineOffset(2)));

    screen.moveCursorTo(LineOffset(1), ColumnOffset(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });

    SECTION("no-op")
    {
        screen.deleteLines(LineCount(0));
        CHECK("AB" == screen.grid().lineText(LineOffset(0)));
        CHECK("CD" == screen.grid().lineText(LineOffset(1)));
        CHECK("EF" == screen.grid().lineText(LineOffset(2)));
    }

    SECTION("in-range")
    {
        logScreenText(screen, "After EL(1) - 1");
        screen.deleteLines(LineCount(1));
        logScreenText(screen, "After EL(1)");
        CHECK("AB" == screen.grid().lineText(LineOffset(0)));
        CHECK("EF" == screen.grid().lineText(LineOffset(1)));
        CHECK("  " == screen.grid().lineText(LineOffset(2)));
    }

    SECTION("clamped")
    {
        screen.moveCursorTo(LineOffset(1), ColumnOffset(1));
        screen.deleteLines(LineCount(5));
        // logScreenText(screen, "After clamped EL(5)");
        CHECK("AB" == screen.grid().lineText(LineOffset(0)));
        CHECK("  " == screen.grid().lineText(LineOffset(1)));
        CHECK("  " == screen.grid().lineText(LineOffset(2)));
    }
}

TEST_CASE("FillArea", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");

    screen.fillArea(L'.', 1, 1, 3, 3);
    CHECK(escape(mainPageText(screen)) == "12345\\n6...0\\nA...E\\nF...J\\nKLMNO\\n");
}

TEST_CASE("DeleteColumns", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.setMode(DECMode::LeftRightMargin, true);
    screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3));
    screen.setTopBottomMargin(LineOffset(1), LineOffset(3));

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    SECTION("outside margin")
    {
        screen.deleteColumns(ColumnCount(1));
        REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("inside margin")
    {
        screen.moveCursorTo(LineOffset(1), ColumnOffset(2));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

        SECTION("DECDC-0")
        {
            screen.deleteColumns(ColumnCount(0));
            REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
        }
        SECTION("DECDC-1")
        {
            screen.deleteColumns(ColumnCount(1));
            REQUIRE("12345\n679 0\nABD E\nFGI J\nKLMNO\n" == screen.renderMainPageText());
        }
        SECTION("DECDC-2")
        {
            screen.deleteColumns(ColumnCount(2));
            REQUIRE("12345\n67  0\nAB  E\nFG  J\nKLMNO\n" == screen.renderMainPageText());
        }
        SECTION("DECDC-3-clamped")
        {
            screen.deleteColumns(ColumnCount(4));
            REQUIRE("12345\n67  0\nAB  E\nFG  J\nKLMNO\n" == screen.renderMainPageText());
        }
    }
}

TEST_CASE("DeleteCharacters", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\033[1;2H");
    REQUIRE("12345\n67890\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    SECTION("outside margin")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3));
        screen.moveCursorTo(LineOffset(0), ColumnOffset(0));
        screen.deleteCharacters(ColumnCount(1));
        REQUIRE("12345\n67890\n" == screen.renderMainPageText());
    }

    SECTION("without horizontal margin")
    {
        SECTION("no-op")
        {
            screen.deleteCharacters(ColumnCount(0));
            REQUIRE("12345\n67890\n" == screen.renderMainPageText());
        }
        SECTION("in-range-1")
        {
            screen.deleteCharacters(ColumnCount(1));
            REQUIRE("1345 \n67890\n" == screen.renderMainPageText());
        }
        SECTION("in-range-2")
        {
            screen.deleteCharacters(ColumnCount(2));
            REQUIRE("145  \n67890\n" == screen.renderMainPageText());
        }
        SECTION("in-range-4")
        {
            screen.deleteCharacters(ColumnCount(4));
            REQUIRE("1    \n67890\n" == screen.renderMainPageText());
        }
        SECTION("clamped")
        {
            screen.deleteCharacters(ColumnCount(5));
            REQUIRE("1    \n67890\n" == screen.renderMainPageText());
        }
    }
    SECTION("with horizontal margin")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset(0), ColumnOffset(3));
        screen.moveCursorTo(LineOffset(0), ColumnOffset(1));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

        SECTION("no-op")
        {
            screen.deleteCharacters(ColumnCount(0));
            REQUIRE("12345\n67890\n" == screen.renderMainPageText());
        }
        SECTION("in-range-1")
        {
            REQUIRE("12345\n67890\n" == screen.renderMainPageText());
            screen.deleteCharacters(ColumnCount(1));
            REQUIRE("134 5\n67890\n" == screen.renderMainPageText());
        }
        SECTION("in-range-2")
        {
            screen.deleteCharacters(ColumnCount(2));
            REQUIRE("14  5\n67890\n" == screen.renderMainPageText());
        }
        SECTION("clamped")
        {
            screen.deleteCharacters(ColumnCount(4));
            REQUIRE("1   5\n67890\n" == screen.renderMainPageText());
        }
    }
}

TEST_CASE("ClearScrollbackBuffer", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) }, LineCount(1) };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO\r\nPQRST\033[H");
    REQUIRE("67890\nABCDE\nFGHIJ\nKLMNO\nPQRST\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
    REQUIRE(screen.historyLineCount() == LineCount(1));
    REQUIRE("12345" == screen.grid().lineText(LineOffset(-1)));

    screen.grid().clearHistory();
    REQUIRE(screen.historyLineCount() == LineCount(0));
}

TEST_CASE("EraseCharacters", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO\033[H");
    logScreenText(screen, "AFTER POPULATE");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    SECTION("ECH-0 equals ECH-1")
    {
        screen.eraseCharacters(ColumnCount(0));
        REQUIRE(" 2345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-1")
    {
        screen.eraseCharacters(ColumnCount(1));
        REQUIRE(" 2345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-2")
    {
        screen.eraseCharacters(ColumnCount(2));
        REQUIRE("  345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-2@2.2")
    {
        screen.moveCursorTo(LineOffset(1), ColumnOffset(1));
        screen.eraseCharacters(ColumnCount(2));
        REQUIRE("12345\n6  90\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-4")
    {
        screen.eraseCharacters(ColumnCount(4));
        REQUIRE("    5\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-5")
    {
        screen.eraseCharacters(ColumnCount(5));
        REQUIRE("     \n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("ECH-6-clamped")
    {
        screen.eraseCharacters(ColumnCount(6));
        REQUIRE("     \n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    }
}

TEST_CASE("ScrollUp.WithMargins")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    logScreenText(screen, "init");
    REQUIRE("12345\n"
            "67890\n"
            "ABCDE\n"
            "FGHIJ\n"
            "KLMNO\n"
            == screen.renderMainPageText());

    // "\033[?69h\033[2;4s\033[2;4r\033[20S"
    screen.setMode(DECMode::LeftRightMargin, true);              // DECSLRM
    screen.setLeftRightMargin(ColumnOffset(1), ColumnOffset(3)); // DECLRMM
    screen.setTopBottomMargin(LineOffset(1), LineOffset(3));     // DECSTBM

    SECTION("SU-1")
    {
        screen.scrollUp(LineCount(1));
        logScreenText(screen, "after 1");
        REQUIRE("12345\n"
                "6BCD0\n"
                "AGHIE\n"
                "F   J\n"
                "KLMNO\n"
                == screen.renderMainPageText());
    }

    SECTION("SU-2")
    {
        screen.scrollUp(LineCount(2));
        logScreenText(screen, "after 2");
        REQUIRE("12345\n"
                "6GHI0\n"
                "A   E\n"
                "F   J\n"
                "KLMNO\n"
                == screen.renderMainPageText());
    }

    SECTION("SU-3")
    {
        screen.scrollUp(LineCount(3));
        logScreenText(screen, "after 3");
        REQUIRE("12345\n"
                "6   0\n"
                "A   E\n"
                "F   J\n"
                "KLMNO\n"
                == screen.renderMainPageText());
    }

    SECTION("SU-3 (overflow)")
    {
        screen.scrollUp(LineCount(4));
        logScreenText(screen, "after 4");
        REQUIRE("12345\n"
                "6   0\n"
                "A   E\n"
                "F   J\n"
                "KLMNO\n"
                == screen.renderMainPageText());
    }
}

TEST_CASE("ScrollUp", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("ABC\r\n");
    screen.write("DEF\r\n");
    screen.write("GHI");
    REQUIRE("ABC\nDEF\nGHI\n" == screen.renderMainPageText());

    SECTION("no-op")
    {
        INFO("begin:");
        screen.scrollUp(LineCount(0));
        INFO("end:");
        REQUIRE("ABC\nDEF\nGHI\n" == screen.renderMainPageText());
    }

    SECTION("by-1")
    {
        screen.scrollUp(LineCount(1));
        REQUIRE("DEF\nGHI\n   \n" == screen.renderMainPageText());
    }

    SECTION("by-2")
    {
        screen.scrollUp(LineCount(2));
        REQUIRE("GHI\n   \n   \n" == screen.renderMainPageText());
    }

    SECTION("by-3")
    {
        screen.scrollUp(LineCount(3));
        REQUIRE("   \n   \n   \n" == screen.renderMainPageText());
    }

    SECTION("clamped")
    {
        screen.scrollUp(LineCount(4));
        REQUIRE("   \n   \n   \n" == screen.renderMainPageText());
    }
}

TEST_CASE("ScrollDown", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    SECTION("scroll fully inside margins")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.setMode(DECMode::Origin, true);

        // SECTION("SD 1") {
        //     screen.scrollDown(LineCount(1));
        //     CHECK("12345\n6   0\nA789E\nFBCDJ\nKLMNO\n" == screen.renderMainPageText());
        // }

        // SECTION("SD 2") {
        //     screen.scrollDown(LineCount(2));
        //     CHECK(
        //         "12345\n"
        //         "6   0\n"
        //         "A   E\n"
        //         "F789J\n"
        //         "KLMNO\n" == screen.renderMainPageText());
        // }
        //
        // SECTION("SD 3") {
        //     screen.scrollDown(LineCount(3));
        //     CHECK(
        //         "12345\n"
        //         "6   0\n"
        //         "A   E\n"
        //         "F   J\n"
        //         "KLMNO\n" == screen.renderMainPageText());
        // }
    }

    SECTION("vertical margins")
    {
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        SECTION("SD 0")
        {
            screen.scrollDown(LineCount(0));
            REQUIRE("12345\n"
                    "67890\n"
                    "ABCDE\n"
                    "FGHIJ\n"
                    "KLMNO\n"
                    == screen.renderMainPageText());
        }

        SECTION("SD 1")
        {
            screen.scrollDown(LineCount(1));
            REQUIRE("12345\n"
                    "     \n"
                    "67890\n"
                    "ABCDE\n"
                    "KLMNO\n"
                    == screen.renderMainPageText());
        }

        SECTION("SD 3")
        {
            screen.scrollDown(LineCount(5));
            REQUIRE("12345\n"
                    "     \n"
                    "     \n"
                    "     \n"
                    "KLMNO\n"
                    == screen.renderMainPageText());
        }

        SECTION("SD 4 clamped")
        {
            screen.scrollDown(LineCount(4));
            REQUIRE("12345\n"
                    "     \n"
                    "     \n"
                    "     \n"
                    "KLMNO\n"
                    == screen.renderMainPageText());
        }
    }

    SECTION("no custom margins")
    {
        SECTION("SD 0")
        {
            screen.scrollDown(LineCount(0));
            REQUIRE("12345\n"
                    "67890\n"
                    "ABCDE\n"
                    "FGHIJ\n"
                    "KLMNO\n"
                    == screen.renderMainPageText());
        }
        SECTION("SD 1")
        {
            screen.scrollDown(LineCount(1));
            REQUIRE("     \n"
                    "12345\n"
                    "67890\n"
                    "ABCDE\n"
                    "FGHIJ\n"
                    == screen.renderMainPageText());
        }
        SECTION("SD 5")
        {
            screen.scrollDown(LineCount(5));
            REQUIRE("     \n"
                    "     \n"
                    "     \n"
                    "     \n"
                    "     \n"
                    == screen.renderMainPageText());
        }
        SECTION("SD 6 clamped")
        {
            screen.scrollDown(LineCount(6));
            REQUIRE("     \n"
                    "     \n"
                    "     \n"
                    "     \n"
                    "     \n"
                    == screen.renderMainPageText());
        }
    }
}

TEST_CASE("MoveCursorUp", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

    SECTION("no-op")
    {
        screen.moveCursorUp(LineCount(0));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });
    }

    SECTION("in-range")
    {
        screen.moveCursorUp(LineCount(1));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
    }

    SECTION("overflow")
    {
        screen.moveCursorUp(LineCount(5));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    }

    SECTION("with margins")
    {
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

        SECTION("in-range")
        {
            screen.moveCursorUp(LineCount(1));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
        }

        SECTION("overflow")
        {
            screen.moveCursorUp(LineCount(5));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
        }
    }

    SECTION("cursor already above margins")
    {
        screen.setTopBottomMargin(LineOffset { 2 }, LineOffset { 3 });
        screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 2 });
        screen.moveCursorUp(LineCount(1));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });
    }
}

TEST_CASE("MoveCursorDown", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(2) } };
    auto& screen = term.screen();
    screen.write("A");
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    // no-op
    screen.moveCursorDown(LineCount(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    // in-range
    screen.moveCursorDown(LineCount(1));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // overflow
    screen.moveCursorDown(LineCount(5));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });
}

TEST_CASE("MoveCursorForward", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    SECTION("no-op")
    {
        screen.moveCursorForward(ColumnCount(0));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
    }

    SECTION("CUF-1")
    {
        screen.moveCursorForward(ColumnCount(1));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    }

    SECTION("CUF-3 (to right border)")
    {
        screen.moveCursorForward(screen.pageSize().columns);
        REQUIRE(screen.logicalCursorPosition().column.value == screen.pageSize().columns.value - 1);
    }

    SECTION("CUF-overflow")
    {
        screen.moveCursorForward(screen.pageSize().columns + ColumnCount(1));
        REQUIRE(screen.logicalCursorPosition().column.value == screen.pageSize().columns.value - 1);
    }
}

TEST_CASE("MoveCursorBackward", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.write("ABC");
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    // no-op
    screen.moveCursorBackward(ColumnCount(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    // in-range
    screen.moveCursorBackward(ColumnCount(1));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    // overflow
    screen.moveCursorBackward(ColumnCount(5));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
}

TEST_CASE("HorizontalPositionAbsolute", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // no-op
    screen.moveCursorToColumn(ColumnOffset(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // in-range
    screen.moveCursorToColumn(ColumnOffset(2));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.moveCursorToColumn(ColumnOffset(1));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    // overflow
    screen.moveCursorToColumn(ColumnOffset(4));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) /*clamped*/ });
}

TEST_CASE("HorizontalPositionRelative", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    SECTION("no-op")
    {
        screen.moveCursorForward(ColumnCount(0));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
    }

    SECTION("HPR-1")
    {
        screen.moveCursorForward(ColumnCount(1));
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    }

    SECTION("HPR-3 (to right border)")
    {
        screen.moveCursorForward(screen.pageSize().columns - 1);
        REQUIRE(screen.logicalCursorPosition().column.value == screen.pageSize().columns.value - 1);
    }

    SECTION("HPR-overflow")
    {
        screen.moveCursorForward(screen.pageSize().columns);
        REQUIRE(screen.logicalCursorPosition().column.value == screen.pageSize().columns.value - 1);
    }
}

TEST_CASE("MoveCursorToColumn", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // no-op
    screen.moveCursorToColumn(ColumnOffset(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // in-range
    screen.moveCursorToColumn(ColumnOffset(2));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.moveCursorToColumn(ColumnOffset(1));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    // overflow
    screen.moveCursorToColumn(ColumnOffset(3));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) /*clamped*/ });

    SECTION("with wide character")
    {
        screen.moveCursorTo({}, {});
        REQUIRE(screen.logicalCursorPosition().column.value == 0);
        screen.write(U"\u26A1"); // ⚡ :flash: (double width)
        REQUIRE(screen.logicalCursorPosition().column.value == 2);
    }
}

TEST_CASE("MoveCursorToLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // no-op
    screen.moveCursorToLine(LineOffset(0));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    // in-range
    screen.moveCursorToLine(LineOffset(2));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(0) });

    screen.moveCursorToLine(LineOffset(1));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });

    // overflow
    screen.moveCursorToLine(LineOffset(3));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2 /*clamped*/), ColumnOffset(0) });
}

TEST_CASE("MoveCursorToBeginOfLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();

    screen.write("\r\nAB");
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    screen.moveCursorToBeginOfLine();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });
}

TEST_CASE("MoveCursorTo", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    SECTION("origin mode disabled")
    {
        SECTION("in range")
        {
            screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });
        }

        SECTION("origin")
        {
            screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 0 });
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        }

        SECTION("clamped")
        {
            screen.moveCursorTo(LineOffset { 5 }, ColumnOffset { 5 });
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(4), ColumnOffset(4) });
        }
    }

    SECTION("origin-mode enabled")
    {
        constexpr auto TopMargin = LineOffset(1);
        constexpr auto BottomMargin = LineOffset(3);
        constexpr auto LeftMargin = ColumnOffset(1);
        constexpr auto RightMargin = ColumnOffset(3);
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(LeftMargin, RightMargin);
        screen.setTopBottomMargin(TopMargin, BottomMargin);
        screen.setMode(DECMode::Origin, true);

        SECTION("move to origin")
        {
            screen.moveCursorTo({}, {});
            CHECK(CellLocation { LineOffset(0), ColumnOffset(0) } == screen.logicalCursorPosition());
            CHECK(CellLocation { LineOffset(1), ColumnOffset(1) } == screen.realCursorPosition());
            CHECK('7' == (char) screen.at({ TopMargin + 0, LeftMargin + 0 }).codepoint(0));
            CHECK('I' == (char) screen.at({ TopMargin + 2, LeftMargin + 2 }).codepoint(0));
        }
    }
}

TEST_CASE("MoveCursorToNextTab", "[screen]")
{
    auto constexpr TabWidth = 8;
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(20) } };
    auto& screen = term.screen();
    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1 * TabWidth + 0) });

    screen.moveCursorToColumn(ColumnOffset(TabWidth - 1));
    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1 * TabWidth + 0) });

    screen.moveCursorToColumn(ColumnOffset(TabWidth - 1));
    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1 * TabWidth + 0) });

    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2 * TabWidth + 0) });

    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(19) });

    screen.setMode(DECMode::AutoWrap, true);
    screen.write("A"); // 'A' is being written at the right margin
    screen.write("B"); // force wrap to next line, writing 'B' at the beginning of the line

    screen.moveCursorToNextTab();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(8) });
}

// TODO: HideCursor
// TODO: ShowCursor

TEST_CASE("SaveCursor and RestoreCursor", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
    auto& screen = term.screen();
    screen.setMode(DECMode::AutoWrap, false);
    screen.saveCursor();

    // mutate the cursor's position, autowrap and origin flags
    screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 2 });
    screen.setMode(DECMode::AutoWrap, true);
    screen.setMode(DECMode::Origin, true);

    // restore cursor and see if the changes have been reverted
    screen.restoreCursor();
    CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
    CHECK_FALSE(screen.isModeEnabled(DECMode::AutoWrap));
    CHECK_FALSE(screen.isModeEnabled(DECMode::Origin));
}

TEST_CASE("Index_outside_margin", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(6), ColumnCount(4) } };
    auto& screen = term.screen();
    screen.write("1234\r\n5678\r\nABCD\r\nEFGH\r\nIJKL\r\nMNOP");
    logScreenText(screen, "initial");
    REQUIRE("1234\n5678\nABCD\nEFGH\nIJKL\nMNOP\n" == screen.renderMainPageText());
    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });

    // with cursor above top margin
    screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 2 });
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

    screen.index();
    REQUIRE("1234\n5678\nABCD\nEFGH\nIJKL\nMNOP\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    // with cursor below bottom margin and above bottom screen (=> only moves cursor one down)
    screen.moveCursorTo(LineOffset { 4 }, ColumnOffset { 2 });
    screen.index();
    REQUIRE("1234\n5678\nABCD\nEFGH\nIJKL\nMNOP\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(5), ColumnOffset(2) });

    // with cursor below bottom margin and at bottom screen (=> no-op)
    screen.moveCursorTo(LineOffset { 5 }, ColumnOffset { 2 });
    screen.index();
    REQUIRE("1234\n5678\nABCD\nEFGH\nIJKL\nMNOP\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(5), ColumnOffset(2) });
}

TEST_CASE("Index_inside_margin", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(6), ColumnCount(2) } };
    auto& screen = term.screen();
    screen.write("11\r\n22\r\n33\r\n44\r\n55\r\n66");
    logScreenText(screen, "initial setup");

    // test IND when cursor is within margin range (=> move cursor down)
    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
    screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });
    screen.index();
    logScreenText(screen, "IND while cursor at line 3");
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });
    REQUIRE("11\n22\n33\n44\n55\n66\n" == screen.renderMainPageText());
}

TEST_CASE("Index_at_bottom_margin", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    logScreenText(screen, "initial setup");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });

    SECTION("cursor at bottom margin and full horizontal margins")
    {
        screen.moveCursorTo(LineOffset { 3 }, ColumnOffset { 1 });
        screen.index();
        logScreenText(screen, "IND while cursor at bottom margin");
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });
        REQUIRE("12345\nABCDE\nFGHIJ\n     \nKLMNO\n" == screen.renderMainPageText());
    }

    SECTION("cursor at bottom margin and NOT full horizontal margins")
    {
        screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 0 });
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.moveCursorTo(LineOffset { 3 }, ColumnOffset { 1 }); // cursor at bottom margin
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });

        screen.index();
        CHECK("12345\n6BCD0\nAGHIE\nF   J\nKLMNO\n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });
    }
}

TEST_CASE("ReverseIndex_without_custom_margins", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    logScreenText(screen, "initial");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    // at bottom screen
    screen.moveCursorTo(LineOffset { 4 }, ColumnOffset { 1 });
    screen.reverseIndex();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });

    screen.reverseIndex();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

    screen.reverseIndex();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    screen.reverseIndex();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    screen.reverseIndex();
    logScreenText(screen, "RI at top screen");
    REQUIRE("     \n12345\n67890\nABCDE\nFGHIJ\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    screen.reverseIndex();
    logScreenText(screen, "RI at top screen");
    REQUIRE("     \n     \n12345\n67890\nABCDE\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
}

TEST_CASE("ReverseIndex_with_vertical_margin", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    logScreenText(screen, "initial");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });

    // below bottom margin
    screen.moveCursorTo(LineOffset { 4 }, ColumnOffset { 1 });
    screen.reverseIndex();
    logScreenText(screen, "RI below bottom margin");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });

    // at bottom margin
    screen.reverseIndex();
    logScreenText(screen, "RI at bottom margin");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

    screen.reverseIndex();
    logScreenText(screen, "RI middle margin");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // at top margin
    screen.reverseIndex();
    logScreenText(screen, "RI at top margin #1");
    REQUIRE("12345\n     \n67890\nABCDE\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // at top margin (again)
    screen.reverseIndex();
    logScreenText(screen, "RI at top margin #2");
    REQUIRE("12345\n     \n     \n67890\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // above top margin
    screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 1 });
    screen.reverseIndex();
    logScreenText(screen, "RI above top margin");
    REQUIRE("12345\n     \n     \n67890\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

    // above top margin (top screen) => no-op
    screen.reverseIndex();
    logScreenText(screen, "RI above top margin (top-screen)");
    REQUIRE("12345\n     \n     \n67890\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
}

TEST_CASE("ReverseIndex_with_vertical_and_horizontal_margin", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    logScreenText(screen, "initial");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    screen.setMode(DECMode::LeftRightMargin, true);
    screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });

    // below bottom margin
    screen.moveCursorTo(LineOffset { 4 }, ColumnOffset { 1 });
    screen.reverseIndex();
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(1) });

    // at bottom margin
    screen.reverseIndex();
    logScreenText(screen, "after RI at bottom margin");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

    screen.reverseIndex();
    logScreenText(screen, "after RI at bottom margin (again)");
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // at top margin
    screen.reverseIndex();
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });
    REQUIRE("12345\n6   0\nA789E\nFBCDJ\nKLMNO\n" == screen.renderMainPageText());

    // at top margin (again)
    screen.reverseIndex();
    logScreenText(screen, "after RI at top margin (again)");
    REQUIRE("12345\n6   0\nA   E\nF789J\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    // above top margin
    screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 1 });
    screen.reverseIndex();
    REQUIRE("12345\n6   0\nA   E\nF789J\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
}

TEST_CASE("ScreenAlignmentPattern", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());

    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

    REQUIRE(1 == *screen.margin().vertical.from);
    REQUIRE(3 == *screen.margin().vertical.to);

    SECTION("test")
    {
        screen.screenAlignmentPattern();
        REQUIRE("EEEEE\nEEEEE\nEEEEE\nEEEEE\nEEEEE\n" == screen.renderMainPageText());

        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });

        REQUIRE(0 == *screen.margin().horizontal.from);
        REQUIRE(4 == *screen.margin().horizontal.to);
        REQUIRE(0 == *screen.margin().vertical.from);
        REQUIRE(4 == *screen.margin().vertical.to);
    }
}

TEST_CASE("CursorNextLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 2 });

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    SECTION("without margins")
    {
        SECTION("normal")
        {
            screen.moveCursorToNextLine(LineCount(1));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(0) });
        }

        SECTION("clamped")
        {
            screen.moveCursorToNextLine(LineCount(5));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(4), ColumnOffset(0) });
        }
    }

    SECTION("with margins")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.setMode(DECMode::Origin, true);
        screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 1 });
        REQUIRE(screen.currentCell().toUtf8() == "8");

        SECTION("normal-1")
        {
            screen.moveCursorToNextLine(LineCount(1));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });
        }

        SECTION("normal-2")
        {
            screen.moveCursorToNextLine(LineCount(2));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(0) });
        }

        SECTION("normal-3")
        {
            screen.moveCursorToNextLine(LineCount(3));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(0) });
        }

        SECTION("clamped-1")
        {
            screen.moveCursorToNextLine(LineCount(4));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(0) });
        }
    }
}

TEST_CASE("CursorPreviousLine", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(4), ColumnOffset(4) });

    SECTION("without margins")
    {
        SECTION("normal")
        {
            screen.moveCursorToPrevLine(LineCount(1));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(3), ColumnOffset(0) });
        }

        SECTION("clamped")
        {
            screen.moveCursorToPrevLine(LineCount(5));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        }
    }

    SECTION("with margins")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.setMode(DECMode::Origin, true);
        screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 2 });
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(2) });

        SECTION("normal-1")
        {
            screen.moveCursorToPrevLine(LineCount(1));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });
        }

        SECTION("normal-2")
        {
            screen.moveCursorToPrevLine(LineCount(2));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        }

        SECTION("clamped")
        {
            screen.moveCursorToPrevLine(LineCount(3));
            REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        }
    }
}

TEST_CASE("ReportCursorPosition", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 2 });

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE("" == term.replyData);
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    SECTION("with Origin mode disabled")
    {
        screen.reportCursorPosition();
        CHECK("\033[2;3R" == term.replyData);
    }

    SECTION("with margins and origin mode enabled")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.setMode(DECMode::Origin, true);
        screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });

        screen.reportCursorPosition();
        CHECK("\033[3;2R" == term.replyData);
    }
}

TEST_CASE("ReportExtendedCursorPosition", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");
    screen.moveCursorTo(LineOffset { 1 }, ColumnOffset { 2 });

    REQUIRE("12345\n67890\nABCDE\nFGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE("" == term.replyData);
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    SECTION("with Origin mode disabled")
    {
        screen.reportExtendedCursorPosition();
        CHECK("\033[2;3;1R" == term.replyData);
    }

    SECTION("with margins and origin mode enabled")
    {
        screen.setMode(DECMode::LeftRightMargin, true);
        screen.setLeftRightMargin(ColumnOffset { 1 }, ColumnOffset { 3 });
        screen.setTopBottomMargin(LineOffset { 1 }, LineOffset { 3 });
        screen.setMode(DECMode::Origin, true);
        screen.moveCursorTo(LineOffset { 2 }, ColumnOffset { 1 });

        screen.reportExtendedCursorPosition();
        CHECK("\033[3;2;1R" == term.replyData);
    }
}

TEST_CASE("SetMode", "[screen]")
{
    SECTION("Auto NewLine Mode: Enabled")
    {
        auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
        auto& screen = term.screen();
        screen.setMode(AnsiMode::AutomaticNewLine, true);
        screen.write("12345\n67890\nABCDE\nFGHIJ\nKLMNO");
        REQUIRE(screen.renderMainPageText() == "12345\n67890\nABCDE\nFGHIJ\nKLMNO\n");
    }

    SECTION("Auto NewLine Mode: Disabled")
    {
        auto term = MockTerm { PageSize { LineCount(3), ColumnCount(3) } };
        auto& screen = term.screen();
        screen.write("A\nB\nC");
        REQUIRE(screen.renderMainPageText() == "A  \n B \n  C\n");
    }
}

TEST_CASE("RequestMode", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(5) } };
    auto& screen = term.screen();

    SECTION("ANSI modes: enabled")
    {
        screen.setMode(AnsiMode::Insert, true); // IRM
        screen.requestAnsiMode((int) AnsiMode::Insert);
        REQUIRE(e(term.replyData) == e(fmt::format("\033[{};1$y", toAnsiModeNum(AnsiMode::Insert))));
    }

    SECTION("ANSI modes: disabled")
    {
        screen.setMode(AnsiMode::Insert, false); // IRM
        screen.requestAnsiMode((int) AnsiMode::Insert);
        REQUIRE(e(term.replyData) == e(fmt::format("\033[{};2$y", toAnsiModeNum(AnsiMode::Insert))));
    }

    SECTION("ANSI modes: unknown")
    {
        AnsiMode m = static_cast<AnsiMode>(1234);
        screen.setMode(m, true); // DECOM
        screen.requestAnsiMode((int) m);
        REQUIRE(e(term.replyData) == e(fmt::format("\033[{};0$y", toAnsiModeNum(m))));
    }

    SECTION("DEC modes: enabled")
    {
        screen.setMode(DECMode::Origin, true); // DECOM
        screen.requestDECMode((int) DECMode::Origin);
        REQUIRE(e(term.replyData) == e(fmt::format("\033[?{};1$y", toDECModeNum(DECMode::Origin))));
    }

    SECTION("DEC modes: disabled")
    {
        screen.setMode(DECMode::Origin, false); // DECOM
        screen.requestDECMode((int) DECMode::Origin);
        REQUIRE(e(term.replyData) == e(fmt::format("\033[?{};2$y", toDECModeNum(DECMode::Origin))));
    }

    SECTION("DEC modes: unknown")
    {
        DECMode m = static_cast<DECMode>(1234);
        screen.setMode(m, true); // DECOM
        screen.requestDECMode(static_cast<int>(m));
        REQUIRE(e(term.replyData) == e(fmt::format("\033[?{};0$y", toDECModeNum(m))));
    }
}

TEST_CASE("peek into history", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(3) }, LineCount { 5 } };
    auto& screen = term.screen();
    screen.write("123\r\n456\r\nABC\r\nDEF");

    REQUIRE("ABC\nDEF\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

    // first line in history
    auto const m1 = screen.grid().lineText(LineOffset(-2));
    CHECK(screen.grid().lineText(LineOffset(-2)) == "123");

    // second line in history
    CHECK(screen.grid().lineText(LineOffset(-1)) == "456");

    // first line on screen buffer
    CHECK(screen.grid().lineText(LineOffset(0)) == "ABC");

    // second line on screen buffer
    CHECK(screen.grid().lineText(LineOffset(1)) == "DEF");

    // out-of-range corner cases
    // CHECK_THROWS(screen.at(LineOffset(2), ColumnOffset(0)));
    // CHECK_THROWS(screen.at(LineOffset(1), ColumnOffset(3)));
    // CHECK_THROWS(screen.at({LineOffset()), ColumnOffset()-1)));
    // XXX currently not checked, as they're intentionally using assert() instead.
}

TEST_CASE("captureBuffer", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(5) }, LineCount { 5 } };
    auto& screen = term.screen();

    //           [...      history ...  ...][main page area]
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");

    SECTION("lines: 0")
    {
        screen.captureBuffer(0, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;\033\\"));
    }
    SECTION("lines: 1")
    {
        screen.captureBuffer(1, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;KLMNO\n\033\\\033]314;\033\\"));
    }
    SECTION("lines: 2")
    {
        screen.captureBuffer(2, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;FGHIJ\nKLMNO\n\033\\\033]314;\033\\"));
    }
    SECTION("lines: 3")
    {
        screen.captureBuffer(3, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;ABCDE\nFGHIJ\nKLMNO\n\033\\\033]314;\033\\"));
    }
    SECTION("lines: 4")
    {
        screen.captureBuffer(4, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;67890\nABCDE\nFGHIJ\nKLMNO\n\033\\\033]314;\033\\"));
    }
    SECTION("lines: 5")
    {
        screen.captureBuffer(5, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;12345\n67890\nABCDE\nFGHIJ\nKLMNO\n\033\\\033]314;\033\\"));
    }
    SECTION("lines: 5 (+1 overflow)")
    {
        screen.captureBuffer(5, false);
        INFO(e(term.replyData));
        CHECK(e(term.replyData) == e("\033]314;12345\n67890\nABCDE\nFGHIJ\nKLMNO\n\033\\\033]314;\033\\"));
    }
}

TEST_CASE("render into history", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(5) }, LineCount { 5 } };
    auto& screen = term.screen();
    screen.write("12345\r\n67890\r\nABCDE\r\nFGHIJ\r\nKLMNO");

    REQUIRE("FGHIJ\nKLMNO\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(4) });
    REQUIRE(screen.historyLineCount() == LineCount { 3 });

    string renderedText;
    renderedText.resize((screen.pageSize().columns + 1).as<size_t>() * screen.pageSize().lines.as<size_t>());

    auto const renderer = [&](Cell const& cell, LineOffset _row, ColumnOffset _column) {
        auto const offset =
            _row.as<size_t>() * (screen.pageSize().columns + 1).as<size_t>() + _column.as<size_t>();
        renderedText.at(offset) = cell.codepointCount() ? static_cast<char>(cell.codepoint(0)) : ' ';
        if (_column == (screen.pageSize().columns - 1).as<ColumnOffset>())
            renderedText.at(offset + 1) = '\n';
    };

    // main area
    logScreenText(screen, "render into history");
    screen.render(renderer);
    REQUIRE("FGHIJ\nKLMNO\n" == renderedText);

    // 1 line into history") {
    fill(renderedText.begin(), renderedText.end(), ' ');
    screen.render(renderer, ScrollOffset { 1 });
    REQUIRE("ABCDE\nFGHIJ\n" == renderedText);

    // 2 lines into history") {
    fill(renderedText.begin(), renderedText.end(), ' ');
    screen.render(renderer, ScrollOffset { 2 });
    REQUIRE("67890\nABCDE\n" == renderedText);

    // 3 lines into history") {
    fill(renderedText.begin(), renderedText.end(), ' ');
    screen.render(renderer, ScrollOffset { 3 });
    REQUIRE("12345\n67890\n" == renderedText);
}

TEST_CASE("HorizontalTabClear.AllTabs", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(5) } };
    auto& screen = term.screen();
    screen.horizontalTabClear(HorizontalTabClear::AllTabs);

    screen.writeText('X');
    screen.moveCursorToNextTab();
    screen.writeText('Y');
    REQUIRE("X   Y" == screen.grid().lineText(LineOffset(0)));

    screen.moveCursorToNextTab();
    screen.writeText('Z');
    REQUIRE("X   Y" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("Z    " == screen.grid().lineText(LineOffset(1)));

    screen.moveCursorToNextTab();
    screen.writeText('A');
    REQUIRE("X   Y" == screen.grid().lineText(LineOffset(0)));
    REQUIRE("Z   A" == screen.grid().lineText(LineOffset(1)));
}

TEST_CASE("HorizontalTabClear.UnderCursor", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) } };
    auto& screen = term.screen();
    screen.setTabWidth(ColumnCount(4));

    // clear tab at column 4
    screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 3 });
    screen.horizontalTabClear(HorizontalTabClear::UnderCursor);

    screen.moveCursorTo({}, {});
    screen.writeText('A');
    screen.moveCursorToNextTab();
    screen.writeText('B');

    //       1234567890
    REQUIRE("A      B  " == screen.grid().lineText(LineOffset(0)));
    REQUIRE("          " == screen.grid().lineText(LineOffset(1)));

    screen.moveCursorToNextTab();
    screen.writeText('C');
    CHECK("A      B C" == screen.grid().lineText(LineOffset(0)));
    CHECK("          " == screen.grid().lineText(LineOffset(1)));
}

TEST_CASE("HorizontalTabSet", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) } };
    auto& screen = term.screen();
    screen.horizontalTabClear(HorizontalTabClear::AllTabs);

    screen.moveCursorToColumn(ColumnOffset(2));
    screen.horizontalTabSet();

    screen.moveCursorToColumn(ColumnOffset(4));
    screen.horizontalTabSet();

    screen.moveCursorToColumn(ColumnOffset(7));
    screen.horizontalTabSet();

    screen.moveCursorToBeginOfLine();

    screen.writeText('1');

    screen.moveCursorToNextTab();
    screen.writeText('3');

    screen.moveCursorToNextTab();
    screen.writeText('5');

    screen.moveCursorToNextTab();
    screen.writeText('8');

    screen.moveCursorToNextTab(); // capped
    screen.writeText('A');        // writes B at right margin, flags for autowrap

    REQUIRE("1 3 5  8 A" == screen.grid().lineText(LineOffset(0)));

    screen.moveCursorToNextTab(); // wrapped
    screen.writeText('B');        // writes B at left margin

    //       1234567890
    REQUIRE("1 3 5  8 A" == screen.grid().lineText(LineOffset(0)));
    screen.moveCursorToNextTab(); // 1 -> 3 (overflow)
    screen.moveCursorToNextTab(); // 3 -> 5
    screen.moveCursorToNextTab(); // 5 -> 8
    screen.writeText('C');

    //     1234567890
    CHECK("1 3 5  8 A" == screen.grid().lineText(LineOffset(0)));
    CHECK("B      C  " == screen.grid().lineText(LineOffset(1)));
}

TEST_CASE("CursorBackwardTab.fixedTabWidth", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) } };
    auto& screen = term.screen();
    screen.setTabWidth(ColumnCount(4)); // 5, 9

    screen.writeText('a');

    screen.moveCursorToNextTab(); // -> 5
    screen.writeText('b');

    screen.moveCursorToNextTab();
    screen.writeText('c'); // -> 9

    //      "1234567890"
    REQUIRE("a   b   c " == screen.grid().lineText(LineOffset(0)));
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(9) });

    SECTION("no op")
    {
        screen.cursorBackwardTab(TabStopCount(0));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(9) });
    }

    SECTION("inside 1")
    {
        screen.cursorBackwardTab(TabStopCount(1));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(8) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("a   b   X " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("inside 2")
    {
        screen.cursorBackwardTab(TabStopCount(2));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(4) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("a   X   c " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("exact")
    {
        screen.cursorBackwardTab(TabStopCount(3));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("X   b   c " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("oveflow")
    {
        screen.cursorBackwardTab(TabStopCount(4));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        screen.writeText('X');
        CHECK("X   b   c " == screen.grid().lineText(LineOffset(0)));
    }
}

TEST_CASE("CursorBackwardTab.manualTabs", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(10) } };
    auto& screen = term.screen();

    screen.moveCursorToColumn(ColumnOffset(4));
    screen.horizontalTabSet();
    screen.moveCursorToColumn(ColumnOffset(8));
    screen.horizontalTabSet();
    screen.moveCursorToBeginOfLine();

    screen.writeText('a');

    screen.moveCursorToNextTab(); // -> 4
    screen.writeText('b');

    screen.moveCursorToNextTab();
    screen.writeText('c'); // -> 8

    //      "1234567890"
    REQUIRE(screen.logicalCursorPosition().column.value == 9);
    REQUIRE("a   b   c " == screen.grid().lineText(LineOffset(0)));

    SECTION("oveflow")
    {
        screen.cursorBackwardTab(TabStopCount(4));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        screen.writeText('X');
        CHECK("X   b   c " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("exact")
    {
        screen.cursorBackwardTab(TabStopCount(3));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(0) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("X   b   c " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("inside 2")
    {
        screen.cursorBackwardTab(TabStopCount(2));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(4) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("a   X   c " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("inside 1")
    {
        screen.cursorBackwardTab(TabStopCount(1));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(8) });
        screen.writeText('X');
        //    "1234567890"
        CHECK("a   b   X " == screen.grid().lineText(LineOffset(0)));
    }

    SECTION("no op")
    {
        screen.cursorBackwardTab(TabStopCount(0));
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(9) });
    }
}

TEST_CASE("findMarkerDownwards", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(4) }, LineCount(10) };
    auto& screen = term.screen();
    REQUIRE_FALSE(screen.findMarkerDownwards(LineOffset(0)).has_value());
    REQUIRE_FALSE(screen.findMarkerDownwards(LineOffset(1)).has_value()); // history bottom
    REQUIRE_FALSE(screen.findMarkerDownwards(LineOffset(2)).has_value());
    REQUIRE_FALSE(screen.findMarkerDownwards(LineOffset(3)).has_value()); // history top
    REQUIRE_FALSE(screen.findMarkerDownwards(LineOffset(4)).has_value()); // overflow

    SECTION("no marks")
    {
        screen.write("1abc"sv); // -3: +
        screen.write("2def"sv); // -2: | history
        screen.write("3ghi"sv); // -1: +
        screen.write("4jkl"sv); //  0: +
        screen.write("5mno"sv); //  1: | main screen
        screen.write("6pqr"sv); //  2: +

        REQUIRE(screen.historyLineCount() == LineCount { 3 });

        // overflow: one above scroll-top
        auto mark = screen.findMarkerDownwards(LineOffset(4));
        REQUIRE_FALSE(mark.has_value());

        // scroll-top
        mark = screen.findMarkerDownwards(LineOffset(3));
        REQUIRE_FALSE(mark.has_value());

        mark = screen.findMarkerDownwards(LineOffset(2));
        REQUIRE_FALSE(mark.has_value());

        mark = screen.findMarkerDownwards(LineOffset(1));
        REQUIRE_FALSE(mark.has_value());

        // underflow: one below scroll buttom
        mark = screen.findMarkerDownwards(LineOffset(0));
        REQUIRE_FALSE(mark.has_value());
    }

    SECTION("with marks")
    {
        // saved lines
        screen.setMark(); // 0 (-3)
        screen.write("1abc\r\n"sv);
        screen.write("2def\r\n"sv); // 1 (-2)
        screen.setMark();
        screen.write("3ghi\r\n"sv); // 2 (-1)

        // visibile screen
        screen.setMark(); // 3 (0)
        screen.write("4jkl\r\n"sv);
        screen.write("5mno\r\n"sv); // 4 (1)
        screen.setMark();           // 5 (2)
        screen.write("6pqr"sv);

        // {{{ pre-expectations
        REQUIRE(screen.grid().lineText(LineOffset(-3)) == "1abc");
        REQUIRE(screen.grid().lineText(LineOffset(-2)) == "2def");
        REQUIRE(screen.grid().lineText(LineOffset(-1)) == "3ghi");

        REQUIRE(screen.grid().lineText(LineOffset(0)) == "4jkl");
        REQUIRE(screen.grid().lineText(LineOffset(1)) == "5mno");
        REQUIRE(screen.grid().lineText(LineOffset(2)) == "6pqr");
        // }}}

        // ======================================================

        // overflow: one above scroll top -> scroll bottom
        // gracefully clamps to scroll-top
        auto marker = screen.findMarkerDownwards(LineOffset(-4));
        REQUIRE(marker.has_value());
        REQUIRE(*marker.value() == -1);

        // scroll top -> scroll bottom
        marker = screen.findMarkerDownwards(LineOffset(-3));
        REQUIRE(marker.has_value());
        REQUIRE(*marker.value() == -1);

        // scroll bottom -> NONE
        marker = screen.findMarkerDownwards(LineOffset(-1));
        REQUIRE(marker.has_value());
        REQUIRE(*marker.value() == 0);
    }
}

TEST_CASE("findMarkerUpwards", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(3), ColumnCount(4) }, LineCount(10) };
    auto& screen = term.screen();
    REQUIRE_FALSE(screen.findMarkerUpwards(LineOffset(-1)).has_value()); // peak into history
    REQUIRE_FALSE(screen.findMarkerUpwards(LineOffset(0)).has_value());
    REQUIRE_FALSE(screen.findMarkerUpwards(LineOffset(1)).has_value());
    REQUIRE_FALSE(screen.findMarkerUpwards(LineOffset(2)).has_value());
    REQUIRE_FALSE(screen.findMarkerUpwards(LineOffset(3)).has_value()); // overflow

    SECTION("no marks")
    {
        screen.write("1abc"sv);
        screen.write("2def"sv);
        screen.write("3ghi"sv);
        screen.write("4jkl"sv);
        screen.write("5mno"sv);
        screen.write("6pqr"sv);

        REQUIRE(screen.historyLineCount() == LineCount { 3 });

        auto mark = screen.findMarkerUpwards(LineOffset(0));
        REQUIRE_FALSE(mark.has_value());

        // bottom line in history
        mark = screen.findMarkerUpwards(LineOffset(1));
        REQUIRE_FALSE(mark.has_value());

        // one above bottom line in history
        mark = screen.findMarkerUpwards(LineOffset(2));
        REQUIRE_FALSE(mark.has_value());

        // top history line
        mark = screen.findMarkerUpwards(LineOffset(3));
        REQUIRE_FALSE(mark.has_value());

        // one above history top
        mark = screen.findMarkerUpwards(LineOffset(4));
        REQUIRE_FALSE(mark.has_value());
    }

    SECTION("with marks")
    {
        // saved lines
        screen.setMark(); // 0 (-3)
        screen.write("1abc\r\n"sv);
        screen.write("2def\r\n"sv); // 1 (-2)
        screen.setMark();
        screen.write("3ghi\r\n"sv); // 2 (-1)

        // visibile screen
        screen.setMark(); // 3 (0)
        screen.write("4jkl\r\n"sv);
        screen.write("5mno\r\n"sv); // 4 (1)
        screen.setMark();           // 5 (2)
        screen.write("6pqr"sv);

        // {{{ pre-checks
        REQUIRE(screen.grid().lineText(LineOffset(-3)) == "1abc"); // marked
        REQUIRE(screen.grid().lineText(LineOffset(-2)) == "2def");
        REQUIRE(screen.grid().lineText(LineOffset(-1)) == "3ghi"); // marked

        REQUIRE(screen.grid().lineText(LineOffset(0)) == "4jkl"); // marked
        REQUIRE(screen.grid().lineText(LineOffset(1)) == "5mno");
        REQUIRE(screen.grid().lineText(LineOffset(2)) == "6pqr"); // marked
        // }}}

        // ======================================================
        // main page top (0) -> scroll offset 1
        auto marker = screen.findMarkerUpwards(LineOffset(0));
        REQUIRE(marker.has_value());
        REQUIRE(marker.value().value == -1); // 3ghi

        // scroll offset 1 -> scroll offset 3
        marker = screen.findMarkerUpwards(LineOffset(-1));
        REQUIRE(marker.has_value());
        REQUIRE(marker.value().value == -3); // 1abc

        // scroll-top
        marker = screen.findMarkerUpwards(LineOffset(-3));
        REQUIRE(!marker.has_value());

        // one-off
        marker = screen.findMarkerUpwards(LineOffset(-4));
        REQUIRE(!marker.has_value());
    }
}

TEST_CASE("DECTABSR", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(35) } };
    auto& screen = term.screen();

    SECTION("default tabstops")
    {
        screen.requestTabStops();
        CHECK(e(term.replyData) == e("\033P2$u1/9/17/25/33\033\\"));
    }

    SECTION("cleared tabs")
    {
        screen.horizontalTabClear(HorizontalTabClear::AllTabs);
        screen.requestTabStops();
        CHECK(e(term.replyData) == e("\033P2$u1/9/17/25/33\033\\"));
    }

    SECTION("custom tabstops")
    {
        screen.horizontalTabClear(HorizontalTabClear::AllTabs);

        screen.moveCursorToColumn(ColumnOffset(1));
        screen.horizontalTabSet();

        screen.moveCursorToColumn(ColumnOffset(3));
        screen.horizontalTabSet();

        screen.moveCursorToColumn(ColumnOffset(7));
        screen.horizontalTabSet();

        screen.moveCursorToColumn(ColumnOffset(15));
        screen.horizontalTabSet();

        screen.requestTabStops();
        CHECK(e(term.replyData) == e("\033P2$u2/4/8/16\033\\"));
    }
}

TEST_CASE("save_restore_DEC_modes", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();

    screen.setMode(DECMode::MouseProtocolHighlightTracking, false);
    screen.saveModes(vector { DECMode::MouseProtocolHighlightTracking });

    screen.setMode(DECMode::MouseProtocolHighlightTracking, true);
    CHECK(screen.isModeEnabled(DECMode::MouseProtocolHighlightTracking));

    screen.restoreModes(vector { DECMode::MouseProtocolHighlightTracking });
    CHECK_FALSE(screen.isModeEnabled(DECMode::MouseProtocolHighlightTracking));
}

TEST_CASE("OSC.2.Unicode")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();

    auto const u32title = u32string_view(U"\U0001F600");
    auto const title = unicode::convert_to<char>(u32title);

    screen.write(U"\033]2;\U0001F600\033\\");
    INFO(term.replyData);
    CHECK(e(term.windowTitle) == e(title));
}

TEST_CASE("OSC.4")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();

    SECTION("query")
    {
        screen.write("\033]4;7;?\033\\");
        INFO(term.replyData);
        REQUIRE(e(term.replyData) == e("\033]4;7;rgb:c0c0/c0c0/c0c0\033\\"));
    }

    SECTION("set color via format rgb:RR/GG/BB")
    {
        screen.write("\033]4;7;rgb:ab/cd/ef\033\\");
        screen.write("\033]4;7;?\033\\");
        INFO(term.replyData);
        REQUIRE(e(term.replyData) == e("\033]4;7;rgb:abab/cdcd/efef\033\\"));
    }

    SECTION("set color via format #RRGGBB")
    {
        screen.write("\033]4;7;#abcdef\033\\");
        screen.write("\033]4;7;?\033\\");
        INFO(e(term.replyData));
        REQUIRE(e(term.replyData) == e("\033]4;7;rgb:abab/cdcd/efef\033\\"));
    }

    SECTION("set color via format #RGB")
    {
        screen.write("\033]4;7;#abc\033\\");
        screen.write("\033]4;7;?\033\\");
        INFO(term.replyData);
        REQUIRE(e(term.replyData) == e("\033]4;7;rgb:a0a0/b0b0/c0c0\033\\"));
    }
}

TEST_CASE("XTGETTCAP")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) } };
    auto& screen = term.screen();
    auto const queryStr = fmt::format("\033P+q{:02X}{:02X}{:02X}\033\\", 'R', 'G', 'B');
    screen.write(queryStr);
    INFO(fmt::format("Reply data: {}", term.replyData));
    // "\033P1+r8/8/8\033\\"
    // TODO: CHECK(...)
}

TEST_CASE("setMaxHistoryLineCount", "[screen]")
{
    // from zero to something
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) }, LineCount(0) };
    auto& screen = term.screen();
    screen.grid().setReflowOnResize(false);
    screen.write("AB\r\nCD");
    REQUIRE("AB\nCD\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    screen.setMaxHistoryLineCount(LineCount(1));
    REQUIRE("AB\nCD\n" == screen.renderMainPageText());
}

// TODO: resize test (should be in Grid_test.cpp?)
TEST_CASE("resize", "[screen]")
{
    auto term = MockTerm { PageSize { LineCount(2), ColumnCount(2) }, LineCount(10) };
    auto& screen = term.screen();
    screen.grid().setReflowOnResize(false);
    screen.write("AB\r\nCD");
    REQUIRE("AB\nCD\n" == screen.renderMainPageText());
    REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

    screen.setMaxHistoryLineCount(LineCount(10));

    SECTION("no-op")
    {
        screen.resize({ LineCount(2), ColumnCount(2) });
        CHECK("AB\nCD\n" == screen.renderMainPageText());
    }

    SECTION("grow lines")
    {
        screen.resize({ LineCount(3), ColumnCount(2) });
        REQUIRE("AB\nCD\n  \n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(1) });

        screen.write("\r\n");
        screen.write("E");
        REQUIRE("AB\nCD\nE \n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });

        screen.write("F");
        REQUIRE("AB\nCD\nEF\n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(2), ColumnOffset(1) });
    }

    SECTION("shrink lines")
    {
        screen.resize({ LineCount(1), ColumnCount(2) });
        CHECK("CD\n" == screen.renderMainPageText());
        CHECK("AB" == screen.grid().lineAt(LineOffset(-1)).toUtf8());
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });
    }

    SECTION("grow columns")
    {
        screen.resize({ LineCount(2), ColumnCount(3) });
        CHECK("AB \nCD \n" == screen.renderMainPageText());
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });
    }

    SECTION("shrink columns")
    {
        screen.resize({ LineCount(2), ColumnCount(1) });
        CHECK("A\nC\n" == screen.renderMainPageText());
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(0) });
    }

    SECTION("regrow columns")
    {
        // 1.) grow
        screen.resize({ LineCount(2), ColumnCount(3) });
        logScreenText(screen, "after columns grow");
        CHECK(screen.logicalCursorPosition() == CellLocation { LineOffset(1), ColumnOffset(2) });

        // 2.) fill
        screen.writeText('Y');
        REQUIRE("AB \nCDY\n" == screen.renderMainPageText());
        screen.moveCursorTo(LineOffset { 0 }, ColumnOffset { 2 });
        screen.writeText('X');
        logScreenText(screen, "after write");
        REQUIRE("ABX\nCDY\n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(2) });

        // 3.) shrink
        screen.resize({ LineCount(2), ColumnCount(2) });
        REQUIRE("AB\nCD\n" == screen.renderMainPageText());
        REQUIRE(screen.logicalCursorPosition() == CellLocation { LineOffset(0), ColumnOffset(1) });

        // 4.) regrow (and see if pre-filled data were retained)
        // NOTE: This is currently not retained. Do we want to recreate this behaviour?
        // screen.resize({LineCount(2), ColumnCount(3)});
        // REQUIRE("ABX\nCDY\n" == screen.renderMainPageText());
        // REQUIRE(screen.logicalCursorPosition() == CellLocation{LineOffset(0), ColumnOffset(2)});
    }

    SECTION("grow rows, grow columns")
    {
        screen.resize({ LineCount(3), ColumnCount(3) });
        REQUIRE("AB \nCD \n   \n" == screen.renderMainPageText());
        screen.write("1\r\n234");
        REQUIRE("AB \nCD1\n234\n" == screen.renderMainPageText());
    }

    SECTION("grow rows, shrink columns")
    {
        screen.resize({ LineCount(3), ColumnCount(1) });
        REQUIRE("A\nC\n \n" == screen.renderMainPageText());
    }

    SECTION("shrink rows, grow columns")
    {
        screen.resize({ LineCount(1), ColumnCount(3) });
        REQUIRE("CD \n" == screen.renderMainPageText());
    }

    SECTION("shrink rows, shrink columns")
    {
        screen.resize({ LineCount(1), ColumnCount(1) });
        REQUIRE("C\n" == screen.renderMainPageText());
    }

    // TODO: what do we want to do when re resize to {0, y}, {x, 0}, {0, 0}?
}

// {{{ DECCRA
// TODO: also verify attributes have been copied
// TODO: also test with: DECOM enabled
// TODO: also test with: margins set and having them exceeded
// TODO: also test with: overflowing source bottom/right dimensions
// TODO: also test with: out-of-bounds target or source top/left positions

MockTerm screenForDECRA()
{
    auto term = MockTerm { PageSize { LineCount(5), ColumnCount(6) } };
    auto& screen = term.screen();

    screen.write("ABCDEF\r\n"
                 "abcdef\r\n"
                 "123456\r\n");
    screen.write("\033[43m");
    screen.write("GHIJKL\r\n"
                 "ghijkl");
    screen.write("\033[0m");

    auto const initialText = "ABCDEF\n"
                             "abcdef\n"
                             "123456\n"
                             "GHIJKL\n"
                             "ghijkl\n";

    CHECK(screen.renderMainPageText() == initialText);

    return term;
}

TEST_CASE("DECCRA.DownLeft.intersecting", "[screen]")
{
    auto term = screenForDECRA();
    auto& screen = term.screen();
    auto const initialText = "ABCDEF\n"
                             "abcdef\n"
                             "123456\n"
                             "GHIJKL\n"
                             "ghijkl\n";
    CHECK(screen.renderMainPageText() == initialText);

    auto constexpr page = 0;

    auto constexpr sTop = 4;
    auto constexpr sLeft = 3;

    auto constexpr sBottom = 5;
    auto constexpr sRight = 6;

    auto constexpr tTop = 3;
    auto constexpr tLeft = 2;

    auto const expectedText = "ABCDEF\n"
                              "abcdef\n" // .3456.
                              "1IJKL6\n" // .IJKL.
                              "GijklL\n"
                              "ghijkl\n";

    // copy up by one line (4 to 3), 2 lines
    // copy left by one column (3 to 2), 2 columns

    auto const deccraSeq =
        fmt::format("\033[{};{};{};{};{};{};{};{}$v", sTop, sLeft, sBottom, sRight, page, tTop, tLeft, page);
    screen.write(deccraSeq);

    auto const resultText = screen.renderMainPageText();
    CHECK(resultText == expectedText);
}

TEST_CASE("DECCRA.Right.intersecting", "[screen]")
{
    // Moves a rectangular area by one column to the right.
    auto term = screenForDECRA();
    auto& screen = term.screen();

    auto const initialText = "ABCDEF\n"
                             "abcdef\n"
                             "123456\n"
                             "GHIJKL\n"
                             "ghijkl\n";
    REQUIRE(screen.renderMainPageText() == initialText);
    auto const expectedText = "ABCDEF\n"
                              "abbcdf\n"
                              "122346\n"
                              "GHHIJL\n"
                              "ghijkl\n";

    auto constexpr page = 0;
    auto constexpr sTopLeft = CellLocation { LineOffset(1), ColumnOffset(1) };
    auto constexpr sBottomRight = CellLocation { LineOffset(3), ColumnOffset(3) };
    auto constexpr tTopLeft = CellLocation { LineOffset(1), ColumnOffset(2) };

    auto const deccraSeq = fmt::format("\033[{};{};{};{};{};{};{};{}$v",
                                       sTopLeft.line + 1,
                                       sTopLeft.column + 1,
                                       sBottomRight.line + 1,
                                       sBottomRight.column + 1,
                                       page,
                                       tTopLeft.line + 1,
                                       tTopLeft.column + 1,
                                       page);
    screen.write(deccraSeq);

    auto const resultText = screen.renderMainPageText();
    CHECK(resultText == expectedText);
}

TEST_CASE("DECCRA.Left.intersecting", "[screen]")
{
    // Moves a rectangular area by one column to the left.
    auto term = screenForDECRA();
    auto& screen = term.screen();
    auto const initialText = "ABCDEF\n"
                             "abcdef\n"
                             "123456\n"
                             "GHIJKL\n"
                             "ghijkl\n";
    CHECK(screen.renderMainPageText() == initialText);

    auto const expectedText = "ABCDEF\n"
                              "abdeff\n"
                              "124566\n"
                              "GHIJKL\n"
                              "ghijkl\n";

    auto constexpr page = 0;
    auto constexpr sTopLeft = CellLocation { LineOffset(1), ColumnOffset(3) };
    auto constexpr sBottomRight = CellLocation { LineOffset(2), ColumnOffset(5) };
    auto constexpr tTopLeft = CellLocation { LineOffset(1), ColumnOffset(2) };

    auto const deccraSeq = fmt::format("\033[{};{};{};{};{};{};{};{}$v",
                                       sTopLeft.line + 1,
                                       sTopLeft.column + 1,
                                       sBottomRight.line + 1,
                                       sBottomRight.column + 1,
                                       page,
                                       tTopLeft.line + 1,
                                       tTopLeft.column + 1,
                                       page);
    screen.write(deccraSeq);

    auto const resultText = screen.renderMainPageText();
    CHECK(resultText == expectedText);
}
// }}}

TEST_CASE("Screen.tcap.string", "[screen, tcap]")
{
    using namespace terminal;
    auto term = MockTerm(PageSize { LineCount(3), ColumnCount(5) }, LineCount(2));
    auto& screen = term.screen();
    screen.write("\033P+q687061\033\\"); // HPA
    REQUIRE(e(term.replyData) == e("\033P1+r687061=1B5B2569257031256447\033\\"));
}

TEST_CASE("Sixel.simple", "[screen]")
{
    auto const pageSize = PageSize { LineCount(10), ColumnCount(10) };
    auto term = MockTerm { pageSize, LineCount(10) };
    term.screen().setCellPixelSize(ImageSize { Width(10), Height(10) });

    auto const sixelData = crispy::readFileAsString("./test/images/squirrel-50.sixel");

    term.screen().write(sixelData);

    CHECK(term.screen().cursor().position.column == ColumnOffset(8));
    CHECK(term.screen().cursor().position.line == LineOffset(4));

    for (auto line = LineOffset(0); line < boxed_cast<LineOffset>(pageSize.lines); ++line)
    {
        for (auto column = ColumnOffset(0); column < boxed_cast<ColumnOffset>(pageSize.columns); ++column)
        {
            Cell const& cell = term.screen().at(line, column);
            if (line <= LineOffset(4) && column <= ColumnOffset(7))
            {
                auto fragment = cell.imageFragment();
                REQUIRE(fragment);
                CHECK(fragment->offset().line == line);
                CHECK(fragment->offset().column == column);
                CHECK(fragment->data().size() != 0);
            }
            else
            {
                CHECK(cell.empty());
            }
        }
    }

    // Um, we could actually test more precise here by validating the grid cell contents.
}

TEST_CASE("Sixel.AutoScroll-1", "[screen]")
{
    // Create a 10x3x5 grid and render a 7x5 image causing one a line-scroll by one.
    auto const pageSize = PageSize { LineCount(4), ColumnCount(10) };
    auto term = MockTerm { pageSize, LineCount(5) };
    term.screen().setCellPixelSize(ImageSize { Width(10), Height(10) });
    term.screen().setMode(DECMode::SixelScrolling, true);

    auto const sixelData = crispy::readFileAsString("./test/images/squirrel-50.sixel");

    term.screen().write(sixelData);

    CHECK(term.screen().cursor().position.column == ColumnOffset(8));
    CHECK(term.screen().cursor().position.line == LineOffset(3));

    for (auto line = LineOffset(-1); line < boxed_cast<LineOffset>(pageSize.lines); ++line)
    {
        INFO(fmt::format("line {}", line));
        for (auto column = ColumnOffset(0); column < boxed_cast<ColumnOffset>(pageSize.columns); ++column)
        {
            INFO(fmt::format("column {}", column));
            Cell const& cell = term.screen().at(line, column);
            if (line <= LineOffset(4) && column <= ColumnOffset(7))
            {
                auto fragment = cell.imageFragment();
                REQUIRE(fragment);
                CHECK(fragment->offset().line == line + 1);
                CHECK(fragment->offset().column == column);
                CHECK(fragment->data().size() != 0);
            }
            else
            {
                CHECK(cell.empty());
            }
        }
    }

    // Um, we could actually test more precise here by validating the grid cell contents.
}

// TODO: Sixel: image that exceeds available lines

// TODO: SetForegroundColor
// TODO: SetBackgroundColor
// TODO: SetGraphicsRendition
// TODO: SetScrollingRegion

// TODO: SendMouseEvents
// TODO: AlternateKeypadMode

// TODO: DesignateCharset
// TODO: SingleShiftSelect

// TODO: ChangeWindowTitle

// TODO: Bell
// TODO: FullReset

// TODO: DeviceStatusReport
// TODO: SendDeviceAttributes
// TODO: SendTerminalId
