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
#include <contour/CaptureScreen.h>
#include <contour/Config.h>
#include <contour/ContourApp.h>

#include "shell_integration_zsh.h"

#include <terminal/Capabilities.h>
#include <terminal/Parser.h>
#include <terminal/Image.h>

#include <crispy/base64.h>
#include <crispy/debuglog.h>
#include <crispy/stdfs.h>
#include <crispy/utils.h>

#include <fstream>
#include <memory>

#define GOOD_IMAGE_PROTOCOL // TODO use cmake here instead

using std::bind;
using std::cout;
using std::ifstream;
using std::make_unique;
using std::ofstream;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace CLI = crispy::cli;

namespace contour {

#if defined(GOOD_IMAGE_PROTOCOL) // {{{
terminal::ImageAlignment parseImageAlignment(string_view _text)
{
    (void) _text;
    return terminal::ImageAlignment::TopStart; // TODO
}

terminal::ImageResize parseImageResize(string_view _text)
{
    (void) _text;
    return terminal::ImageResize::NoResize; // TODO
}

terminal::Coordinate parsePosition(string_view _text)
{
    (void) _text;
    return {}; // TODO
}

// TODO: chunkedFileReader(path) to return iterator over spans of data chunks.
std::vector<uint8_t> readFile(FileSystem::path const& _path)
{
    auto ifs = ifstream(_path.string());
    if (!ifs.good())
        return {};

    auto const size = FileSystem::file_size(_path);
    auto text = vector<uint8_t>();
    text.resize(size);
    ifs.read((char*) &text[0], size);
    return text;
}

void displayImage(terminal::ImageResize _resizePolicy,
                  terminal::ImageAlignment _alignmentPolicy,
                  crispy::Size _screenSize,
                  string_view _fileName)
{
    auto constexpr ST = "\033\\"sv;

    cout << fmt::format("{}f={},c={},l={},a={},z={};",
        "\033Ps"sv,             // GIONESHOT
        '0',                    // image format: 0 = auto detect
        _screenSize.width,
        _screenSize.height,
        int(_alignmentPolicy),
        int(_resizePolicy)
    );

#if 1
    auto const data = readFile(_fileName);// TODO: incremental buffered read
    auto encoderState = crispy::base64::EncoderState{};

    vector<char> buf;
    auto const writer = [&](string_view _data) { for (auto ch: _data) buf.push_back(ch); };
    auto const flush = [&]() { cout.write(buf.data(), buf.size()); buf.clear(); };

    for (uint8_t const byte: data)
    {
        crispy::base64::encode(byte, encoderState, writer);
        if (buf.size() >= 4096)
            flush();
    }
    flush();
#endif

    cout << ST;
}
#endif // }}}

ContourApp::ContourApp() :
    App("contour", "Contour Terminal Emulator", CONTOUR_VERSION_STRING)
{
    link("contour.capture", bind(&ContourApp::captureAction, this));
    link("contour.list-debug-tags", bind(&ContourApp::listDebugTagsAction, this));
    link("contour.set.profile", bind(&ContourApp::profileAction, this));
    link("contour.parser-table", bind(&ContourApp::parserTableAction, this));
    link("contour.generate.terminfo", bind(&ContourApp::terminfoAction, this));
    link("contour.generate.config", bind(&ContourApp::configAction, this));
    link("contour.generate.integration", bind(&ContourApp::integrationAction, this));
}

template <typename Callback>
auto withOutput(crispy::cli::FlagStore const& _flags, std::string const& _name, Callback _callback)
{
    std::ostream* out = &cout;

    auto const& outputFileName = _flags.get<string>(_name); // TODO: support string_view
    auto ownedOutput = unique_ptr<std::ostream>{};
    if (outputFileName != "-")
    {
        ownedOutput = make_unique<std::ofstream>(outputFileName);
        out = ownedOutput.get();
    }

    return _callback(*out);
}

int ContourApp::integrationAction()
{
    return withOutput(parameters(), "contour.generate.integration.to", [&](auto& _stream) {
        auto const shell = parameters().get<string>("contour.generate.integration.shell");
        if (shell == "zsh")
        {
            _stream.write((char const*) shell_integration_zsh.data(), shell_integration_zsh.size());
            return EXIT_SUCCESS;
        }
        else
        {
            std::cerr << fmt::format("Cannot generate shell integration for an unsupported shell, {}.\n", shell);
            return EXIT_FAILURE;
        }
    });
}

int ContourApp::configAction()
{
    withOutput(parameters(), "contour.generate.config.to", [](auto& _stream) {
        _stream << config::createDefaultConfig();
    });
    return EXIT_SUCCESS;
}

int ContourApp::terminfoAction()
{
    withOutput(parameters(), "contour.generate.terminfo.to", [](auto& _stream) {
        _stream << terminal::capabilities::StaticDatabase{}.terminfo();
    });
    return EXIT_SUCCESS;
}

int ContourApp::captureAction()
{
    auto captureSettings = contour::CaptureSettings{};
    captureSettings.logicalLines = parameters().get<bool>("contour.capture.logical");
    captureSettings.timeout = parameters().get<double>("contour.capture.timeout");
    captureSettings.lineCount = parameters().get<unsigned>("contour.capture.lines");
    captureSettings.outputFile = parameters().get<string>("contour.capture.to");

    if (contour::captureScreen(captureSettings))
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

int ContourApp::parserTableAction()
{
    terminal::parser::dot(std::cout, terminal::parser::ParserTable::get());
    return EXIT_SUCCESS;
}

int ContourApp::listDebugTagsAction()
{
    listDebugTags();
    return EXIT_SUCCESS;
}

int ContourApp::profileAction()
{
    auto const profileName = parameters().get<string>("contour.set.profile.to");
    // TODO: guard `profileName` value against invalid input.
    cout << fmt::format("\033P$p{}\033\\", profileName);
    return EXIT_SUCCESS;
}

#if defined(GOOD_IMAGE_PROTOCOL)
crispy::Size parseSize(string_view _text)
{
    (void) _text;
    return crispy::Size{};//TODO
}

int ContourApp::imageAction()
{
    auto const resizePolicy = parseImageResize(parameters().get<string>("contour.image.resize"));
    auto const alignmentPolicy = parseImageAlignment(parameters().get<string>("contour.image.align"));
    auto const size = parseSize(parameters().get<string>("contour.image.size"));
    auto const fileName = parameters().verbatim.front();
    // TODO: how do we wanna handle more than one verbatim arg (image)?
    // => report error and EXIT_FAILURE as only one verbatim arg is allowed.
    // FIXME: What if parameter `size` is given as `_size` instead, it should cause an
    //        invalid-argument error above already!
    displayImage(resizePolicy, alignmentPolicy, size, fileName);
    return EXIT_SUCCESS;
}
#endif

crispy::cli::Command ContourApp::parameterDefinition() const
{
    return CLI::Command{
        "contour",
        "Contour Terminal Emulator " CONTOUR_VERSION_STRING " - https://github.com/christianparpart/contour/ ;-)",
        CLI::OptionList{},
        CLI::CommandList{
            CLI::Command{"help", "Shows this help and exits."},
            CLI::Command{"version", "Shows The version and exits."},
            CLI::Command{"parser-table", "Dumps parser table"},
            CLI::Command{"list-debug-tags", "Lists all available debug tags and exits."},
            CLI::Command{
                "generate",
                "Generation utilities.",
                CLI::OptionList{},
                CLI::CommandList{
                    CLI::Command{
                        "terminfo",
                        "Generates the terminfo source file that will reflect the features of this version of contour. Using - as value will write to stdout instead.",
                        {
                            CLI::Option{
                                "to",
                                CLI::Value{""s},
                                "Output file name to store the screen capture to. If - (dash) is given, the output will be written to standard output.",
                                "FILE",
                                CLI::Presence::Required
                            },
                        }
                    },
                    CLI::Command{
                        "config",
                        "Generates configuration file with the default configuration.",
                        CLI::OptionList{
                            CLI::Option{
                                "to",
                                CLI::Value{""s},
                                "Output file name to store the config file to. If - (dash) is given, the output will be written to standard output.",
                                "FILE",
                                CLI::Presence::Required
                            },
                        }
                    },
                    CLI::Command{
                        "integration",
                        "Generates shell integration script.",
                        CLI::OptionList{
                            CLI::Option{
                                "shell",
                                CLI::Value{""s},
                                "Shell name to create the integration for. Currently only zsh is supported.",
                                "SHELL",
                                CLI::Presence::Required
                            },
                            CLI::Option{
                                "to",
                                CLI::Value{""s},
                                "Output file name to store the shell integration file to. If - (dash) is given, the output will be written to standard output.",
                                "FILE",
                                CLI::Presence::Required
                            },
                        }
                    }
                }
            },
#if defined(GOOD_IMAGE_PROTOCOL)
            CLI::Command{
                "image",
                "Sends an image to the terminal emulator for display.",
                CLI::OptionList{
                    CLI::Option{"resize", CLI::Value{"fit"s},
                        "Sets the image resize policy.\n"
                        "Policies available are:\n"
                        " - no (no resize),\n"
                        " - fit (resize to fit),\n"
                        " - fill (resize to fill),\n"
                        " - stretch (stretch to fill)."
                    },
                    CLI::Option{"align", CLI::Value{"center"s},
                        "Sets the image alignment policy.\n"
                        "Possible policies are: TopLeft, TopCenter, TopRight, MiddleLeft, MiddleCenter, MiddleRight, BottomLeft, BottomCenter, BottomRight."
                    },
                    CLI::Option{"size", CLI::Value{""s},
                        "Sets the amount of columns and rows to place the image onto. "
                        "The top-left of the this area is the current cursor position, "
                        "and it will be scrolled automatically if not enough rows are present."
                    }
                },
                CLI::CommandList{},
                CLI::CommandSelect::Explicit,
                CLI::Verbatim{
                    "IMAGE_FILE",
                    "Path to image to be displayed. Image formats supported are at least PNG, JPG."
                }
            },
#endif
            CLI::Command{
                "capture",
                "Captures the screen buffer of the currently running terminal.",
                {
                    CLI::Option{"logical", CLI::Value{false}, "Tells the terminal to use logical lines for counting and capturing."},
                    CLI::Option{"timeout", CLI::Value{1.0}, "Sets timeout seconds to wait for terminal to respond.", "SECONDS"},
                    CLI::Option{"lines", CLI::Value{0u}, "The number of lines to capture", "COUNT"},
                    CLI::Option{"to", CLI::Value{""s}, "Output file name to store the screen capture to. If - (dash) is given, the capture will be written to standard output.", "FILE", CLI::Presence::Required},
                }
            },
            CLI::Command{
                "set",
                "Sets various aspects of the connected terminal.",
                CLI::OptionList{},
                CLI::CommandList{
                    CLI::Command{
                        "profile",
                        "Changes the terminal profile of the currently attached terminal to the given value.",
                        CLI::OptionList{
                            CLI::Option{"to", CLI::Value{""s}, "Profile name to activate in the currently connected terminal.", "NAME"}
                        }
                    }
                }
            }
        }
    };
}

}
