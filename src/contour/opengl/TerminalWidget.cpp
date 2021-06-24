/**
 * This file is part of the "contour" project
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
#include <contour/opengl/TerminalWidget.h>
#include <contour/opengl/OpenGLRenderer.h>

#include <contour/Actions.h>
#include <contour/helper.h>

#include <terminal/Color.h>
#include <terminal/Metrics.h>
#include <terminal/pty/Pty.h>

#include <crispy/debuglog.h>
#include <crispy/stdfs.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtNetwork/QHostInfo>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QKeyEvent>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#if defined(CONTOUR_BLUR_PLATFORM_KWIN)
#include <KWindowEffects>
#endif

#include <algorithm>
#include <cstring>
// #include <execution>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

#include <range/v3/all.hpp>

// Temporarily disabled (I think it was OS/X that didn't like glDebugMessageCallback).
// #define CONTOUR_DEBUG_OPENGL 1

using crispy::Point;
using crispy::Size;
using crispy::Zero;

using namespace std::string_literals;
using namespace std;
using std::chrono::steady_clock;

using namespace std::string_view_literals;

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCDNAME__
#endif

namespace contour::opengl {

using namespace std;
using actions::Action;

#define DUMP_COUNTER(text) \
    do { \
        static uint64_t i = 0; \
        printf("%s/%lu\n", (text), ++i); \
    } while (0)

namespace // {{{
{
#if !defined(NDEBUG) && defined(GL_DEBUG_OUTPUT) && defined(CONTOUR_DEBUG_OPENGL)
    void glMessageCallback(
        GLenum _source,
        GLenum _type,
        [[maybe_unused]] GLuint _id,
        GLenum _severity,
        [[maybe_unused]] GLsizei _length,
        GLchar const* _message,
        [[maybe_unused]] void const* _userParam)
    {
        string const sourceName = [&]() {
            switch (_source)
            {
#if defined(GL_DEBUG_SOURCE_API_ARB)
                case GL_DEBUG_SOURCE_API_ARB:
                    return "API"s;
#endif
#if defined(GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
                    return "window system"s;
#endif
#if defined(GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
                case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
                    return "shader compiler"s;
#endif
#if defined(GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
                case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
                    return "third party"s;
#endif
#if defined(GL_DEBUG_SOURCE_APPLICATION_ARB)
                case GL_DEBUG_SOURCE_APPLICATION_ARB:
                    return "application"s;
#endif
#if defined(GL_DEBUG_SOURCE_OTHER_ARB)
                case GL_DEBUG_SOURCE_OTHER_ARB:
                    return "other"s;
#endif
                default:
                    return fmt::format("{}", _severity);
            }
        }();
        string const typeName = [&]() {
            switch (_type)
            {
#if defined(GL_DEBUG_TYPE_ERROR)
                case GL_DEBUG_TYPE_ERROR:
                    return "error"s;
#endif
#if defined(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                    return "deprecated"s;
#endif
#if defined(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                    return "undefined"s;
#endif
#if defined(GL_DEBUG_TYPE_PORTABILITY)
                case GL_DEBUG_TYPE_PORTABILITY:
                    return "portability"s;
#endif
#if defined(GL_DEBUG_TYPE_PERFORMANCE)
                case GL_DEBUG_TYPE_PERFORMANCE:
                    return "performance"s;
#endif
#if defined(GL_DEBUG_TYPE_OTHER)
                case GL_DEBUG_TYPE_OTHER:
                    return "other"s;
#endif
                default:
                    return fmt::format("{}", _severity);
            }
        }();
        string const debugSeverity = [&]() {
            switch (_severity)
            {
#if defined(GL_DEBUG_SEVERITY_LOW)
                case GL_DEBUG_SEVERITY_LOW:
                    return "low"s;
#endif
#if defined(GL_DEBUG_SEVERITY_MEDIUM)
                case GL_DEBUG_SEVERITY_MEDIUM:
                    return "medium"s;
#endif
#if defined(GL_DEBUG_SEVERITY_HIGH)
                case GL_DEBUG_SEVERITY_HIGH:
                    return "high"s;
#endif
#if defined(GL_DEBUG_SEVERITY_NOTIFICATION)
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    return "notification"s;
#endif
                default:
                    return fmt::format("{}", _severity);
            }
        }();
        auto const tag = []([[maybe_unused]] GLint _type) {
            switch (_type)
            {
            #ifdef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED";
            #endif
            #ifdef GL_DEBUG_TYPE_MARKER
                case GL_DEBUG_TYPE_MARKER: return "MARKER";
            #endif
            #ifdef GL_DEBUG_TYPE_OTHER
                case GL_DEBUG_TYPE_OTHER: return "OTHER";
            #endif
            #ifdef GL_DEBUG_TYPE_PORTABILITY
                case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
            #endif
            #ifdef GL_DEBUG_TYPE_PERFORMANCE
                case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
            #endif
            #ifdef GL_DEBUG_TYPE_ERROR
                case GL_DEBUG_TYPE_ERROR: return "ERROR";
            #endif
                default:
                    return "UNKNOWN";
            }
        }(_type);

        debuglog(WidgetTag).write(
            "[OpenGL/{}]: type:{}, source:{}, severity:{}; {}",
            tag, typeName, sourceName, debugSeverity, _message
        );
    }
#endif

    std::string unhandledExceptionMessage(std::string_view const& where, exception const& e)
    {
        return fmt::format("{}: Unhandled exception caught ({}). {}", where, typeid(e).name(), e.what());
    }

    void reportUnhandledException(std::string_view const& where, exception const& e)
    {
        debuglog(WidgetTag).write("{}", unhandledExceptionMessage(where, e));
        cerr << unhandledExceptionMessage(where, e) << endl;
    }

    QScreen* screenOf(QWidget const* _widget)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        return _widget->screen();
#elif QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        #warning "Using alternative implementation of screenOf() for Qt >= 5.10.0"
        if (auto topLevel = _widget->window())
        {
            if (auto screenByPos = QGuiApplication::screenAt(topLevel->geometry().center()))
                return screenByPos;
        }
        return QGuiApplication::primaryScreen();
#else
        #warning "Using alternative implementation of screenOf() for Qt < 5.10.0"
        return QGuiApplication::primaryScreen();
#endif
    }
} // }}}

terminal::renderer::FontDescriptions TerminalWidget::sanitizeDPI(terminal::renderer::FontDescriptions _fonts)
{
    if (_fonts.dpi.x <= 0 || _fonts.dpi.y <= 0)
        _fonts.dpi = screenDPI();
    return _fonts;
}

TerminalWidget::TerminalWidget(
    config::TerminalProfile const& _profile,
    TerminalSession& _session,
    function<void()> _adaptSize,
    function<void(bool)> _enableBackgroundBlur
):
    QOpenGLWidget(),
    profile_{ _profile },
    session_{ _session },
    adaptSize_{ std::move(_adaptSize) },
    enableBackgroundBlur_{ std::move(_enableBackgroundBlur) },
    windowMargin_{ computeMargin(profile_.terminalSize, {width(), height()}) },
    renderer_{
        terminal().screenSize(),
        sanitizeDPI(profile_.fonts),
        terminal().screen().colorPalette(),
        profile_.backgroundOpacity,
        profile_.hyperlinkDecoration.normal,
        profile_.hyperlinkDecoration.hover
        //TODO: , WindowMargin(windowMargin_.left, windowMargin_.bottom);
    },
    size_{
        static_cast<int>(terminal().screenSize().width * gridMetrics().cellSize.width),
        static_cast<int>(terminal().screenSize().height * gridMetrics().cellSize.height)
    }
{
    debuglog(WidgetTag).write("ctor: terminalSize={}, fontSize={}, contentScale={}, geometry={}:{}..{}:{}",
                              profile_.terminalSize,
                              profile_.fonts.size,
                              contentScale(),
                              geometry().top(),
                              geometry().left(),
                              geometry().bottom(),
                              geometry().right());

    setMouseTracking(true);
    setFormat(surfaceFormat());

    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_OpaquePaintEvent);

    setMinimumSize(gridMetrics().cellSize.width * 3,
                   gridMetrics().cellSize.height * 2);

    // setAttribute(Qt::WA_TranslucentBackground);
    // setAttribute(Qt::WA_NoSystemBackground, false);

    updateTimer_.setSingleShot(true);
    connect(&updateTimer_, &QTimer::timeout, this, QOverload<>::of(&TerminalWidget::blinkingCursorUpdate));

    connect(this, SIGNAL(frameSwapped()), this, SLOT(onFrameSwapped()));

    // TODO
    // configureTerminal(view(), config(), actionHandler_->profileName());
    QOpenGLWidget::updateGeometry();
}

TerminalWidget::~TerminalWidget()
{
    debuglog(WidgetTag).write("TerminalWidget.dtor!");
    makeCurrent(); // XXX must be called.
}

// {{{ attributes
double TerminalWidget::refreshRate() const
{
    auto const screen = screenOf(this);
    if (!screen)
        return profile_.refreshRate != 0.0 ? profile_.refreshRate : 30.0;

    auto const systemRefreshRate = static_cast<double>(screen->refreshRate());
    if (1.0 < profile_.refreshRate && profile_.refreshRate < systemRefreshRate)
        return profile_.refreshRate;
    else
        return systemRefreshRate;
}

crispy::Point TerminalWidget::screenDPI() const
{
    return crispy::Point{ logicalDpiX(), logicalDpiY() };
}

bool TerminalWidget::isFullScreen() const
{
    return window()->isFullScreen();
}

crispy::Size TerminalWidget::pixelSize() const
{
    return size_;
}

crispy::Size TerminalWidget::cellSize() const
{
    return gridMetrics().cellSize;
}
// }}}

// {{{ OpenGL render API
QSurfaceFormat TerminalWidget::surfaceFormat()
{
    QSurfaceFormat format;

    constexpr bool forceOpenGLES = (
#if defined(__linux__)
        true
#else
        false
#endif
    );

    if (forceOpenGLES || QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES)
    {
        format.setVersion(3, 2);
        format.setRenderableType(QSurfaceFormat::OpenGLES);
    }
    else
    {
        format.setVersion(3, 3);
        format.setRenderableType(QSurfaceFormat::OpenGL);
    }
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setAlphaBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);

#if !defined(NDEBUG)
    format.setOption(QSurfaceFormat::DebugContext);
#endif

    return format;
}

QSize TerminalWidget::minimumSizeHint() const
{
    auto constexpr MinimumScreenSize = Size{3, 2};

    auto const cellSize = gridMetrics().cellSize;
    auto const viewSize = MinimumScreenSize * cellSize;

    return QSize(viewSize.width, viewSize.height);
}

QSize TerminalWidget::sizeHint() const
{
    auto const cellSize = renderer_.gridMetrics().cellSize;
    auto const viewSize = cellSize * profile_.terminalSize;


    debuglog(WidgetTag).write("sizeHint: {}, cellSize: {}, terminalSize: {}, dpi: {}",
                              viewSize, cellSize, profile_.terminalSize,
                              renderer_.fontDescriptions().dpi
                              );

    return QSize(viewSize.width, viewSize.height);
}

void TerminalWidget::initializeGL()
{
    initializeOpenGLFunctions();

    renderTarget_ = make_unique<terminal::renderer::opengl::OpenGLRenderer>(
        *config::Config::loadShaderConfig(config::ShaderClass::Text),
        *config::Config::loadShaderConfig(config::ShaderClass::Background),
        Size{width(), height()},
        0, // TODO left margin
        0 // TODO bottom margin
    );

    renderer_.setRenderTarget(*renderTarget_);

    // {{{ some info
    static bool infoPrinted = false;
    if (!infoPrinted)
    {
        infoPrinted = true;
        debuglog(WidgetTag).write("[FYI] DPI             : {} physical; {} logical",
                         crispy::Size{physicalDpiX(), physicalDpiY()},
                         crispy::Size{logicalDpiX(), logicalDpiY()});
        auto const fontSizeInPx = int(ceil(profile_.fonts.size.pt / 72.0 * 96.0 * contentScale()));
        debuglog(WidgetTag).write("[FYI] Font size       : {} ({}px)", profile_.fonts.size, fontSizeInPx);
        debuglog(WidgetTag).write("[FYI] OpenGL type     : {}", (QOpenGLContext::currentContext()->isOpenGLES() ? "OpenGL/ES" : "OpenGL"));
        debuglog(WidgetTag).write("[FYI] OpenGL renderer : {}", glGetString(GL_RENDERER));
        debuglog(WidgetTag).write("[FYI] Qt platform     : {}", QGuiApplication::platformName().toStdString());

        GLint versionMajor{};
        GLint versionMinor{};
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
        debuglog(WidgetTag).write("[FYI] OpenGL version  : {}.{}", versionMajor, versionMinor);

        auto glslVersionMsg = fmt::format("[FYI] GLSL version    : {}", glGetString(GL_SHADING_LANGUAGE_VERSION));

        // TODO: pass phys()/logical?) dpi to font manager, so font size can be applied right
        // TODO: also take window monitor switches into account

        GLint glslNumShaderVersions{};
#if defined(GL_NUM_SHADING_LANGUAGE_VERSIONS)
        glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &glslNumShaderVersions);
        glGetError(); // consume possible OpenGL error.
        if (glslNumShaderVersions > 0)
        {
            glslVersionMsg += " (";
            for (GLint k = 0, l = 0; k < glslNumShaderVersions; ++k)
                if (auto const str = glGetStringi(GL_SHADING_LANGUAGE_VERSION, k); str && *str)
                {
                    glslVersionMsg += (l ? ", " : "");
                    glslVersionMsg += (char const*) str;
                    l++;
                }
            glslVersionMsg += ')';
        }
#endif
        debuglog(WidgetTag).write(glslVersionMsg);
    }
    // }}}

#if !defined(NDEBUG) && defined(GL_DEBUG_OUTPUT) && defined(CONTOUR_DEBUG_OPENGL)
    CHECKED_GL( glEnable(GL_DEBUG_OUTPUT) );
    CHECKED_GL( glDebugMessageCallback(&glMessageCallback, this) );
#endif

    initialized_ = true;
    session_.displayInitialized();
}

void TerminalWidget::resizeGL(int _width, int _height)
{
    debuglog(WidgetTag).write("resizing to {}", Size{_width, _height});
    QOpenGLWidget::resizeGL(_width, _height);

    if (_width == 0 || _height == 0)
        return;

    size_ = Size{_width, _height};
    auto const newScreenSize = screenSize();

    windowMargin_ = computeMargin(newScreenSize, size_);
    renderer_.setRenderSize(size_);
    renderer_.setScreenSize(newScreenSize);
    renderer_.setMargin(windowMargin_.left, windowMargin_.bottom);
    //renderer_.clearCache();

    if (newScreenSize != terminal().screenSize())
    {
        terminal().resizeScreen(newScreenSize, newScreenSize * gridMetrics().cellSize);
        terminal().clearSelection();
    }
}

void TerminalWidget::paintGL()
{
    try
    {
        [[maybe_unused]] auto const lastState = state_.exchange(State::CleanPainting);

#if 0
        auto const updateCount = stats_.updatesSinceRendering.exchange(0);
        auto const renderCount = stats_.consecutiveRenderCount.exchange(0);
        debuglog(WidgetTag).write(
            "paintGL#{}: {} updates since last paint (state: {}).",
            renderCount,
            updateCount,
            lastState
        );
#endif

        bool const reverseVideo =
            terminal().screen().isModeEnabled(terminal::DECMode::ReverseVideo);

        auto const bg =
            reverseVideo
                ? terminal::RGBAColor(profile_.colors.defaultForeground, uint8_t(profile_.backgroundOpacity))
                : terminal::RGBAColor(profile_.colors.defaultBackground, uint8_t(profile_.backgroundOpacity));

        if (bg != renderStateCache_.backgroundColor)
        {
            auto const clearColor = array<float, 4>{
                float(bg.red()) / 255.0f,
                float(bg.green()) / 255.0f,
                float(bg.blue()) / 255.0f,
                float(bg.alpha()) / 255.0f
            };
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
            renderStateCache_.backgroundColor = bg;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        renderer_.render(terminal(), steady_clock::now(), renderingPressure_);
    }
    catch (exception const& e)
    {
        reportUnhandledException(__PRETTY_FUNCTION__, e);
    }
}

void TerminalWidget::onFrameSwapped()
{
    for (;;)
    {
        auto state = state_.load();
        switch (state)
        {
            case State::DirtyIdle:
                //assert(!"The impossible happened, painting but painting. Shakesbeer.");
                //qDebug() << "The impossible happened, onFrameSwapped() called in wrong state DirtyIdle.";
                update();
                return;
            case State::DirtyPainting:
                stats_.consecutiveRenderCount++;
                update();
                return;
            case State::CleanPainting:
                if (!state_.compare_exchange_strong(state, State::CleanIdle))
                    break;
                [[fallthrough]];
            case State::CleanIdle:
                renderingPressure_ = false;
                if (profile_.cursorDisplay == terminal::CursorDisplay::Blink
                        && terminal().cursorVisibility())
                    updateTimer_.start(terminal().nextRender(steady_clock::now()));
                return;
        }
    }
}
// }}}

// {{{ Input handling
void TerminalWidget::keyPressEvent(QKeyEvent* _keyEvent)
{
   sendKeyEvent(_keyEvent, session_);
}

void TerminalWidget::wheelEvent(QWheelEvent* _event)
{
    sendWheelEvent(_event, session_);
}

void TerminalWidget::mousePressEvent(QMouseEvent* _event)
{
    sendMousePressEvent(_event, session_);
}

void TerminalWidget::mouseMoveEvent(QMouseEvent* _event)
{
    sendMouseMoveEvent(_event, session_);
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    sendMouseReleaseEvent(_event, session_);
}

void TerminalWidget::focusInEvent(QFocusEvent* _event)
{
    QOpenGLWidget::focusInEvent(_event);
    session_.sendFocusInEvent(); // TODO: paint with "normal" colors
}

void TerminalWidget::focusOutEvent(QFocusEvent* _event)
{
    QOpenGLWidget::focusOutEvent(_event);
    session_.sendFocusOutEvent(); // TODO maybe paint with "faint" colors
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent* _event)
{
    if (!_event->commitString().isEmpty())
    {
        QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, _event->commitString());
        keyPressEvent(&keyEvent);
        // TODO: emit keyPressedSignal(&keyEvent);
    }

    // if (_readOnly && isCursorOnDisplay())
    // {
    //     // _inputMethodData.preeditString = event->preeditString();
    //     // update(preeditRect() | _inputMethodData.previousPreeditRect);
    // }

    _event->accept();
}

QVariant TerminalWidget::inputMethodQuery(Qt::InputMethodQuery _query) const
{
    const QPoint cursorPos = QPoint(); // TODO: cursorPosition();
    switch (_query) {
    // TODO?: case Qt::ImCursorRectangle:
    // case Qt::ImMicroFocus:
    //     return imageToWidget(QRect(cursorPos.x(), cursorPos.y(), 1, 1));
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        // return the cursor position within the current line
        return cursorPos.x();
    // case Qt::ImSurroundingText:
    // {
    //     // return the text from the current line
    //     QString lineText;
    //     QTextStream stream(&lineText);
    //     PlainTextDecoder decoder;
    //     decoder.begin(&stream);
    //     if (isCursorOnDisplay()) {
    //         decoder.decodeLine(&_image[loc(0, cursorPos.y())], _usedColumns, LINE_DEFAULT);
    //     }
    //     decoder.end();
    //     return lineText;
    // }
    case Qt::ImCurrentSelection:
        return QString();
    default:
        break;
    }

    return QVariant();
}

bool TerminalWidget::event(QEvent* _event)
{
    try
    {
        //qDebug() << "TerminalWidget.event():" << _event;
        if (_event->type() == QEvent::Close)
        {
            session_.pty().close();
            emit terminated();
        }

        return QOpenGLWidget::event(_event);
    }
    catch (std::exception const& e)
    {
        reportUnhandledException(__PRETTY_FUNCTION__, e);
        return false;
    }
}
// }}}

// {{{ (user requested) actions
void TerminalWidget::post(std::function<void()> _fn)
{
    postToObject(this, std::move(_fn));
}

bool TerminalWidget::requestPermission(config::Permission _allowedByConfig, string_view _topicText)
{
    return contour::requestPermission(rememberedPermissions_,
                                      this,
                                      _allowedByConfig,
                                      _topicText);
}

terminal::FontDef TerminalWidget::getFontDef()
{
    auto const fontByStyle = [&](text::font_weight _weight, text::font_slant _slant) -> text::font_description const&
    {
        auto const bold = _weight != text::font_weight::normal;
        auto const italic = _slant != text::font_slant::normal;
        if (bold && italic)
            return renderer_.fontDescriptions().boldItalic;
        else if (bold)
            return renderer_.fontDescriptions().bold;
        else if (italic)
            return renderer_.fontDescriptions().italic;
        else
            return renderer_.fontDescriptions().regular;
    };
    auto const nameOfStyledFont = [&](text::font_weight _weight, text::font_slant _slant) -> string
    {
        auto const& regularFont = renderer_.fontDescriptions().regular;
        auto const& styledFont = fontByStyle(_weight, _slant);
        if (styledFont.familyName == regularFont.familyName)
            return "auto";
        else
            return styledFont.toPattern();
    };
    return {
        renderer_.fontDescriptions().size.pt,
        renderer_.fontDescriptions().regular.familyName,
        nameOfStyledFont(text::font_weight::bold, text::font_slant::normal),
        nameOfStyledFont(text::font_weight::normal, text::font_slant::italic),
        nameOfStyledFont(text::font_weight::bold, text::font_slant::italic),
        renderer_.fontDescriptions().emoji.toPattern()
    };
}

void TerminalWidget::bell()
{
}

void TerminalWidget::copyToClipboard(std::string_view _data)
{
    if (QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr)
        clipboard->setText(QString::fromUtf8(_data.data(), static_cast<int>(_data.size())));
}

void TerminalWidget::dumpState()
{
    makeCurrent();
    auto const tmpDir = FileSystem::path(QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString());
    auto const targetDir = tmpDir / FileSystem::path("contour-debug");
    FileSystem::create_directories(targetDir);
    debuglog(WidgetTag).write("Dumping state into directory: {}", targetDir.generic_string());
    // TODO: The above should be done from the outside and the targetDir being passed into this call.
    // TODO: maybe zip this dir in the end.

    // TODO: use this file store for everything that needs to be dumped.
    terminal().screen().dumpState("Dump screen state.");
    renderer_.dumpState(std::cout);

    enum class ImageBufferFormat { RGBA, RGB, Alpha };

    auto screenshotSaver = [](FileSystem::path const& _filename, ImageBufferFormat _format) {
        auto const [qImageFormat, elementCount] = [&]() -> tuple<QImage::Format, int> {
            switch (_format) {
                case ImageBufferFormat::RGBA: return tuple{QImage::Format_RGBA8888, 4};
                case ImageBufferFormat::RGB: return tuple{QImage::Format_RGB888, 3};
                case ImageBufferFormat::Alpha: return tuple{QImage::Format_Grayscale8, 1};
            }
            return tuple{QImage::Format_Grayscale8, 1};
        }();

        // That's a little workaround for MacOS/X's C++ Clang compiler.
        auto const theImageFormat = qImageFormat;
        auto const theElementCount = elementCount;

        return [_filename, theImageFormat, theElementCount](vector<uint8_t> const& _buffer, Size _size) {
            auto image = make_unique<QImage>(_size.width, _size.height, theImageFormat);
            // Vertically flip the image, because the coordinate system between OpenGL and desktop screens is inverse.
            crispy::for_each(
                // TODO: std::execution::seq,
                crispy::times(_size.height),
                [&_buffer, &image, theElementCount, _size](int i) {
                    uint8_t const* sourceLine = &_buffer.data()[i * _size.width * theElementCount];
                    copy(sourceLine, sourceLine + _size.width * theElementCount, image->scanLine(_size.height - i - 1));
                }
            );
            image->save(QString::fromStdString(_filename.generic_string()));
        };
    };

    auto const atlasScreenshotSaver = [&screenshotSaver, &targetDir](std::string const& _allocatorName,
                                                                     unsigned _instanceId,
                                                                     vector<uint8_t> const& _buffer,
                                                                     Size _size) {
        return [&screenshotSaver, &targetDir, &_buffer, _size, _allocatorName, _instanceId](ImageBufferFormat _format) {
            auto const formatText = [&]() {
                switch (_format) {
                    case ImageBufferFormat::RGBA: return "rgba"sv;
                    case ImageBufferFormat::RGB: return "rgb"sv;
                    case ImageBufferFormat::Alpha: return "alpha"sv;
                }
                return "unknown"sv;
            }();
            auto const fileName = targetDir / fmt::format("atlas-{}-{}-{}.png", _allocatorName, formatText, _instanceId);
            return screenshotSaver(fileName, _format)(_buffer, _size);
        };
    };

    terminal::renderer::RenderTarget& renderTarget = renderer_.renderTarget();

    for (auto const* allocator: renderTarget.allAtlasAllocators())
    {
        for (auto const atlasID: allocator->activeAtlasTextures())
        {
            auto infoOpt = renderTarget.readAtlas(*allocator, atlasID);
            if (!infoOpt.has_value())
                continue;

            terminal::renderer::AtlasTextureInfo& info = infoOpt.value();
            auto const saveScreenshot = atlasScreenshotSaver(allocator->name(), atlasID.value, info.buffer, info.size);
            switch (info.format)
            {
                case terminal::renderer::atlas::Format::RGBA:
                    saveScreenshot(ImageBufferFormat::RGBA);
                    break;
                case terminal::renderer::atlas::Format::RGB:
                    saveScreenshot(ImageBufferFormat::RGB);
                    break;
                case terminal::renderer::atlas::Format::Red:
                    saveScreenshot(ImageBufferFormat::Alpha);
                    break;
            }
        }
    }

    renderTarget.scheduleScreenshot(screenshotSaver(targetDir / "screenshot.png", ImageBufferFormat::RGBA));
}

void TerminalWidget::notify(std::string_view /*_title*/, std::string_view /*_body*/)
{
    // TODO: showNotification callback to Controller?
}

void TerminalWidget::resizeWindow(int _width, int _height, bool _inPixels)
{
    if (isFullScreen())
    {
        debuglog(WidgetTag).write("Application request to resize window in full screen mode denied.");
        return;
    }

    auto requestedScreenSize = terminal().screenSize();

    if (_inPixels)
    {
        auto const pixelSize = crispy::Size{
            _width ? _width : width(),
            _height ? _height : height()
        };
        requestedScreenSize = pixelSize / gridMetrics().cellSize;
    }
    else
    {
        if (_width)
            requestedScreenSize.width = _width;

        if (_height)
            requestedScreenSize.height = _height;
    }

    //setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    const_cast<config::TerminalProfile&>(profile_).terminalSize = requestedScreenSize;
    renderer_.setScreenSize(requestedScreenSize);
    terminal().resizeScreen(requestedScreenSize, requestedScreenSize * gridMetrics().cellSize);
    updateGeometry();
    adaptSize_();
}

void TerminalWidget::setFonts(terminal::renderer::FontDescriptions _fontDescriptions)
{
    if (renderer_.fontDescriptions() == _fontDescriptions)
        return;

    windowMargin_ = computeMargin(screenSize(), size_);
    auto fd = _fontDescriptions;
    if (fd.dpi == Zero<Point>)
        fd.dpi = screenDPI();
    renderer_.setFonts(std::move(fd));//_fontDescriptions);
    renderer_.setMargin(windowMargin_.left, windowMargin_.bottom);
    renderer_.updateFontMetrics();

    // resize widget (same pixels, but adjusted terminal rows/columns and margin)
    resize(size_);
}

bool TerminalWidget::setFontSize(text::font_size _size)
{
    if (_size.pt < 5.) // Let's not be crazy.
        return false;

    if (_size.pt > 200.)
        return false;

    if (!renderer_.setFontSize(_size))
        return false;

    windowMargin_ = computeMargin(screenSize(), pixelSize());
    renderer_.setMargin(windowMargin_.left, windowMargin_.bottom);

    // resize terminalView (same pixels, but adjusted terminal rows/columns and margin)
    resize(size_);

    updateMinimumSize();
    return true;
}

bool TerminalWidget::setScreenSize(crispy::Size _newScreenSize)
{
    if (_newScreenSize == terminal().screenSize())
        return false;

    renderer_.setScreenSize(_newScreenSize);
    terminal().resizeScreen(_newScreenSize, _newScreenSize * cellSize());
    return true;
}

constexpr Qt::CursorShape toQtMouseShape(MouseCursorShape _shape)
{
    switch (_shape)
    {
        case contour::MouseCursorShape::Hidden:
            return Qt::CursorShape::BlankCursor;
        case contour::MouseCursorShape::Arrow:
            return Qt::CursorShape::ArrowCursor;
        case contour::MouseCursorShape::IBeam:
            return Qt::CursorShape::IBeamCursor;
        case contour::MouseCursorShape::PointingHand:
            return Qt::CursorShape::PointingHandCursor;
    }

    // should never be reached
    return Qt::CursorShape::ArrowCursor;
}

void TerminalWidget::setMouseCursorShape(MouseCursorShape _shape)
{
    auto const newShape = toQtMouseShape(_shape);
    if (cursor().shape() == newShape)
        return;

    setCursor(newShape);
}

void TerminalWidget::setTerminalProfile(config::TerminalProfile _profile)
{
    (void) _profile;
    // TODO
    // profile_ = std::move(_profile);
}

void TerminalWidget::setWindowTitle(string_view _title)
{
    auto const title = _title.empty()
        ? "contour"s
        : fmt::format("{} - contour", _title);

    // TODO: since we do not control the whole window, it would be best to emit a signal (or call back) instead.
    if (window() && window()->windowHandle())
        window()->windowHandle()->setTitle(QString::fromUtf8(title.c_str()));
}

void TerminalWidget::setWindowFullScreen()
{
    assertInitialized();
    window()->windowHandle()->showFullScreen();
}

void TerminalWidget::setWindowMaximized()
{
    assertInitialized();
    window()->showMaximized();
    maximizedState_ = true;
}

void TerminalWidget::setWindowNormal()
{
    assertInitialized();
    updateMinimumSize();
    window()->windowHandle()->showNormal();
    maximizedState_ = false;
}

void TerminalWidget::setBackgroundBlur(bool _enable)
{
    if (!enableBackgroundBlur_)
        return;

    enableBackgroundBlur_(_enable);
}

void TerminalWidget::toggleFullScreen()
{
    assertInitialized();

    if (window()->isFullScreen())
    {
        window()->showNormal();
        if (maximizedState_)
            window()->showMaximized();
    }
    else
    {
        maximizedState_ = window()->isMaximized();
        window()->showFullScreen();
    }

    // if (window_.visibility() == QWindow::FullScreen)
    //     window_.setVisibility(QWindow::Windowed);
    // else
    //     window_.setVisibility(QWindow::FullScreen);
}

void TerminalWidget::setHyperlinkDecoration(terminal::renderer::Decorator _normal,
                                            terminal::renderer::Decorator _hover)
{
    renderer_.setHyperlinkDecoration(_normal, _hover);
}

void TerminalWidget::setBackgroundOpacity(terminal::Opacity _opacity)
{
    renderer_.setBackgroundOpacity(_opacity);
    session_.terminal().breakLoopAndRefreshRenderBuffer();
}
// }}}

// {{{ terminal events
void TerminalWidget::scheduleRedraw()
{
    if (!initialized_.load())
        return;

    if (setScreenDirty())
    {
        update(); //QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));

        emit terminalBufferUpdated(); // TODO: should not be invoked, as it's not guarranteed to be updated.
    }
}

void TerminalWidget::renderBufferUpdated()
{
    scheduleRedraw();
}

void TerminalWidget::onClosed()
{
    post([this]() { close(); });
}

void TerminalWidget::onSelectionCompleted()
{
    if (QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr)
    {
        string const text = terminal().extractSelectionText();
        clipboard->setText(QString::fromUtf8(text.c_str(), static_cast<int>(text.size())), QClipboard::Selection);
    }
}

void TerminalWidget::bufferChanged(terminal::ScreenType _type)
{
    using Type = terminal::ScreenType;
    switch (_type)
    {
        case Type::Main:
            setCursor(Qt::IBeamCursor);
            break;
        case Type::Alternate:
            setCursor(Qt::ArrowCursor);
            break;
    }
    emit terminalBufferChanged(_type);
    //scheduleRedraw();
}

void TerminalWidget::discardImage(terminal::Image const& _image)
{
    renderer_.discardImage(_image);
}
// }}}

// {{{ helpers
void TerminalWidget::assertInitialized()
{
    if (initialized_)
        return;

    throw std::runtime_error(
        "Internal error. "
        "TerminalWidget function invoked before initialization has finished."
    );
}

void TerminalWidget::onScrollBarValueChanged(int _value)
{
    terminal().viewport().scrollToAbsolute(_value);
    scheduleRedraw();
}

void TerminalWidget::blinkingCursorUpdate()
{
    scheduleRedraw();
}

TerminalWidget::WindowMargin TerminalWidget::computeMargin(Size _charCells, Size _pixels) const noexcept
{
    auto const usedHeight = static_cast<int>(_charCells.height * gridMetrics().cellSize.height);
    auto const freeHeight = static_cast<int>(_pixels.height - usedHeight);
    auto const bottomMargin = freeHeight;

    //auto const usedWidth = _charCells.columns * regularFont_.maxAdvance();
    //auto const freeWidth = _pixels.width - usedWidth;
    auto constexpr leftMargin = 0;

    return {leftMargin, bottomMargin};
}

float TerminalWidget::contentScale() const
{
    if (!window()->windowHandle())
        return 1.0f;

    return window()->windowHandle()->screen()->devicePixelRatio();
}

void TerminalWidget::resize(Size _size)
{
    size_ = _size;

    auto const newScreenSize = screenSize();

    windowMargin_ = computeMargin(newScreenSize, size_);

    renderer_.setRenderSize(_size);
    renderer_.setScreenSize(newScreenSize);
    renderer_.setMargin(windowMargin_.left, windowMargin_.bottom);
    //renderer_.clearCache();

    if (newScreenSize != terminal().screenSize())
    {
        terminal().resizeScreen(newScreenSize, newScreenSize * gridMetrics().cellSize);
        terminal().clearSelection();
    }
}

void TerminalWidget::updateMinimumSize()
{
    auto const MinimumGridSize = Size{3, 2};
    auto const minSize = gridMetrics().cellSize * MinimumGridSize;
    setMinimumSize(minSize.width, minSize.height);
}
// }}}

} // namespace contour
