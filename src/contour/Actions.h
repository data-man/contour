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
#pragma once

#include <optional>
#include <string>
#include <variant>

#include <fmt/format.h>

namespace contour::actions {

struct FollowHyperlink{};
struct ToggleAllKeyMaps{};
struct ToggleFullscreen{};
struct ScreenshotVT{};
struct IncreaseFontSize{};
struct DecreaseFontSize{};
struct IncreaseOpacity{};
struct DecreaseOpacity{};
struct SendChars{ std::string chars; };
struct WriteScreen{ std::string chars; }; // "\033[2J\033[3J"
struct ScrollOneUp{};
struct ScrollOneDown{};
struct ScrollUp{};
struct ScrollDown{};
struct ScrollPageUp{};
struct ScrollPageDown{};
struct ScrollMarkUp{};
struct ScrollMarkDown{};
struct ScrollToTop{};
struct ScrollToBottom{};
struct PasteClipboard{};
struct CopySelection{};
struct PasteSelection{};
struct ChangeProfile{ std::string name; };
struct NewTerminal{ std::optional<std::string> profileName; };
struct OpenConfiguration{};
struct OpenFileManager{};
struct Quit{};
struct ResetFontSize{};
struct ReloadConfig{ std::optional<std::string> profileName; };
struct ResetConfig{};
struct CopyPreviousMarkRange{};
// CloseTab
// OpenTab
// FocusNextTab
// FocusPreviousTab

using Action = std::variant<
    FollowHyperlink,
    ResetFontSize,
    ReloadConfig,
    ResetConfig,
    ToggleAllKeyMaps,
    ToggleFullscreen,
    ScreenshotVT,
    IncreaseFontSize,
    DecreaseFontSize,
    IncreaseOpacity,
    DecreaseOpacity,
    SendChars,
    WriteScreen,
    ScrollOneUp,
    ScrollOneDown,
    ScrollUp,
    ScrollDown,
    ScrollPageUp,
    ScrollPageDown,
    ScrollMarkUp,
    ScrollMarkDown,
    ScrollToTop,
    ScrollToBottom,
    CopySelection,
    PasteSelection,
    PasteClipboard,
    ChangeProfile,
    NewTerminal,
    OpenConfiguration,
    OpenFileManager,
    Quit,
    CopyPreviousMarkRange
>;

std::optional<Action> fromString(std::string const& _name);

} // namespace contour::actions

// {{{ fmtlib custom formatters
#define DECLARE_ACTION_FMT(T) \
    namespace fmt { \
        template <> \
        struct formatter<contour::actions:: T> { \
            template <typename ParseContext> \
            constexpr auto parse(ParseContext& ctx) { return ctx.begin(); } \
            template <typename FormatContext> \
            auto format(contour::actions:: T const&, FormatContext& ctx) \
            { \
                return format_to(ctx.out(), "{}", #T); \
            } \
        }; \
    }

// {{{ declare
DECLARE_ACTION_FMT(ChangeProfile);
DECLARE_ACTION_FMT(CopyPreviousMarkRange);
DECLARE_ACTION_FMT(CopySelection);
DECLARE_ACTION_FMT(DecreaseFontSize);
DECLARE_ACTION_FMT(DecreaseOpacity);
DECLARE_ACTION_FMT(FollowHyperlink);
DECLARE_ACTION_FMT(IncreaseFontSize);
DECLARE_ACTION_FMT(IncreaseOpacity);
DECLARE_ACTION_FMT(NewTerminal);
DECLARE_ACTION_FMT(OpenConfiguration);
DECLARE_ACTION_FMT(OpenFileManager);
DECLARE_ACTION_FMT(PasteClipboard);
DECLARE_ACTION_FMT(PasteSelection);
DECLARE_ACTION_FMT(Quit);
DECLARE_ACTION_FMT(ReloadConfig);
DECLARE_ACTION_FMT(ResetConfig);
DECLARE_ACTION_FMT(ResetFontSize);
DECLARE_ACTION_FMT(ScreenshotVT);
DECLARE_ACTION_FMT(ScrollDown);
DECLARE_ACTION_FMT(ScrollMarkDown);
DECLARE_ACTION_FMT(ScrollMarkUp);
DECLARE_ACTION_FMT(ScrollOneDown);
DECLARE_ACTION_FMT(ScrollOneUp);
DECLARE_ACTION_FMT(ScrollPageDown);
DECLARE_ACTION_FMT(ScrollPageUp);
DECLARE_ACTION_FMT(ScrollToBottom);
DECLARE_ACTION_FMT(ScrollToTop);
DECLARE_ACTION_FMT(ScrollUp);
DECLARE_ACTION_FMT(SendChars);
DECLARE_ACTION_FMT(ToggleAllKeyMaps);
DECLARE_ACTION_FMT(ToggleFullscreen);
DECLARE_ACTION_FMT(WriteScreen);
// }}}
#undef DECLARE_ACTION_FMT

#define HANDLE_ACTION(T) \
    if (std::holds_alternative<contour::actions:: T>(_action)) \
    { \
        contour::actions:: T const& a = std::get<contour::actions:: T>(_action); \
        return format_to(ctx.out(), "{}", a); \
    }

namespace fmt {
    template <>
    struct formatter<contour::actions::Action> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(contour::actions::Action const& _action, FormatContext& ctx)
        {
            // {{{ handle
            HANDLE_ACTION(ChangeProfile);
            HANDLE_ACTION(CopyPreviousMarkRange);
            HANDLE_ACTION(CopySelection);
            HANDLE_ACTION(DecreaseFontSize);
            HANDLE_ACTION(DecreaseOpacity);
            HANDLE_ACTION(FollowHyperlink);
            HANDLE_ACTION(IncreaseFontSize);
            HANDLE_ACTION(IncreaseOpacity);
            HANDLE_ACTION(NewTerminal);
            HANDLE_ACTION(OpenConfiguration);
            HANDLE_ACTION(OpenFileManager);
            HANDLE_ACTION(PasteClipboard);
            HANDLE_ACTION(PasteSelection);
            HANDLE_ACTION(Quit);
            HANDLE_ACTION(ReloadConfig);
            HANDLE_ACTION(ResetConfig);
            HANDLE_ACTION(ResetFontSize);
            HANDLE_ACTION(ScreenshotVT);
            HANDLE_ACTION(ScrollDown);
            HANDLE_ACTION(ScrollMarkDown);
            HANDLE_ACTION(ScrollMarkUp);
            HANDLE_ACTION(ScrollOneDown);
            HANDLE_ACTION(ScrollOneUp);
            HANDLE_ACTION(ScrollPageDown);
            HANDLE_ACTION(ScrollPageUp);
            HANDLE_ACTION(ScrollToBottom);
            HANDLE_ACTION(ScrollToTop);
            HANDLE_ACTION(ScrollUp);
            HANDLE_ACTION(SendChars);
            HANDLE_ACTION(ToggleAllKeyMaps);
            HANDLE_ACTION(ToggleFullscreen);
            HANDLE_ACTION(WriteScreen);
            // }}}
            return format_to(ctx.out(), "UNKNOWN ACTION");
        }
    };
}
#undef HANDLE_ACTION
// ]}}
