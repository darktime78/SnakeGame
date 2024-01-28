#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

static std::pair<std::uint16_t, std::uint16_t> GetTerminalDimensions() {
  struct winsize terminalSize;
  // get the width and height of the terminal.
  ioctl(0, TIOCGWINSZ, &terminalSize);
  return {terminalSize.ws_col, terminalSize.ws_row};
}

class Control {
  static constexpr auto BufferSize{3};
  std::array<char, BufferSize> m_buffer{0};
  ssize_t m_bufferLength{0};
  int m_epollFd{-1};

public:
  static constexpr std::uint8_t Enter{0xa};
  static constexpr std::uint8_t UpArrow{0x41};
  static constexpr std::uint8_t DownArrow{0x42};
  static constexpr std::uint8_t RightArrow{0x43};
  static constexpr std::uint8_t LeftArrow{0x44};
  static constexpr std::uint8_t Escape{0x1b};

public:
  Control() : m_epollFd{epoll_create1(0)} {
    if (m_epollFd < 0) {
      perror("epoll_create1");
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
  };

  ~Control() { close(m_epollFd); }

  int ReadKey() {
    struct epoll_event events[1];
    int nfds = epoll_wait(m_epollFd, events, 1, 1);
    if (nfds <= 0 && events[0].data.fd != STDIN_FILENO) {
      return -1;
    }

    m_bufferLength = read(STDIN_FILENO, &m_buffer, BufferSize);
    if (m_bufferLength <= 0) {

      return -1;
    }

    char code1 = m_buffer[0], code2 = m_buffer[1], code3 = m_buffer[2];
    memset(&m_buffer, 0, BufferSize);
    // check the codes.
    if (code1 == Escape and code2 == 0x5b and code3 == UpArrow)
      return UpArrow;
    else if (code1 == Escape and code2 == 0x5b and code3 == DownArrow)
      return DownArrow;
    else if (code1 == Escape and code2 == 0x5b and code3 == RightArrow)
      return RightArrow;
    else if (code1 == Escape and code2 == 0x5b and code3 == LeftArrow)
      return LeftArrow;
    else if ((code1 == Escape or code1 == 'q') and code2 == 0x0 and
             code3 == 0x0)
      return Escape;
    else if (code1 == Enter and code2 == 0x0 and code3 == 0x0)
      return Enter;
    else
      return -1;
  }
};

class Screen {
  struct termios m_oldTerminal;
  std::uint16_t m_columns{0};
  std::uint16_t m_rows{0};

  void ClearScreen() {
    printf("\033[2J");
    printf("\033[H");
  }
  void MoveCursor(std::uint16_t x, std::uint16_t y) {
    printf("\033[%d;%dH", y, x);
  }

public:
  Screen() = default;
  explicit Screen(std::uint16_t xDimension, std::uint16_t yDimension)
      : m_columns{xDimension}, m_rows{yDimension} {
    // Hide cursor
    printf("\033[?25l");
    // canonical mode and disable echo
    struct termios m_newTerminal;
    if (tcgetattr(STDIN_FILENO, &m_oldTerminal) == -1) {
      perror("tcgetattr");
    };
    m_newTerminal = m_oldTerminal;
    m_newTerminal.c_lflag &= ~(ICANON | ECHO);
    m_newTerminal.c_cc[VTIME] = 0;
    m_newTerminal.c_cc[VMIN] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &m_newTerminal) == -1) {
      perror("tcsetattr");
    };
  }

  ~Screen() {
    // Show cursor
    printf("\033[?25h");

    if (tcsetattr(STDIN_FILENO, TCSANOW, &m_oldTerminal) == -1) {
      perror("tcsetattr");
    };
  }

  void Show(std::uint16_t xOffset, std::uint16_t yOffset, const char *data) {
    printf("\033[%i;%iH%s", yOffset, xOffset, data);
  }

  void ShowGameField() {
    ClearScreen();
    printf("┌");
    for (int i = 0; i <= m_columns - 3; i++) {
      printf("─");
    }
    printf("┐\n");

    for (int j = 0; j <= m_rows - 4; j++) {
      printf("│");
      MoveCursor(m_columns, j + 2);
      printf("│\n");
    }

    printf("└");
    for (int i = 0; i <= m_columns - 3; i++)
      printf("─");
    printf("┘");
    printf("Scope : 0");
  }
  void ShowStartGame() {
    static constexpr auto DyOffsetText{20};
    static constexpr auto StartGameLine1{
        "______   __    __   ______   __    __  ________\n "};
    static constexpr auto StartGameLine2{
        "/      \\ |  \\  |  \\ /      \\ |  \\  /  \\|        \\ \n"};
    static constexpr auto StartGameLine3{
        "/  $$$$$$\\| $$\\ | $$|  $$$$$$\\| $$ /  $$| $$$$$$$$  \n"};
    static constexpr auto StartGameLine4{
        "\\ $$___\\$$| $$$\\| $$| $$__| $$| $$/  $$ | $$__      \n"};
    static constexpr auto StartGameLine5{
        "\\$$    \\ | $$$$\\ $$| $$    $$| $$  $$  | $$  \\    \n"};
    static constexpr auto StartGameLine6{
        "_\\$$$$$$\\| $$\\$$ $$| $$$$$$$$| $$$$$\\  | $$$$$    \n"};
    static constexpr auto StartGameLine7{
        "\\  \\__| $$| $$ \\$$$$| $$  | $$| $$ \\$$\\ | $$_____   \n"};
    static constexpr auto StartGameLine8{
        "\\$$    $$| $$  \\$$$| $$  | $$| $$  \\$$\\| $$     \\ \n"};
    static constexpr auto StartGameLine9{
        "  \\$$$$$$  \\$$   \\$$ \\$$   \\$$ \\$$   \\$$ \\$$$$$$$$ \n\n\n"};
    static constexpr auto StartGameLine10{"Press Enter to Start New Game \n"};

    ClearScreen();
    unsigned short dy = (m_rows - DyOffsetText) / 2;
    unsigned short dx = (m_columns - strlen(StartGameLine1)) / 2;
    Show(dx, dy, StartGameLine1);
    dx = (m_columns - strlen(StartGameLine2)) / 2;
    Show(dx, dy + 1, StartGameLine2);
    dx = (m_columns - strlen(StartGameLine3)) / 2;
    Show(dx, dy + 2, StartGameLine3);
    dx = (m_columns - strlen(StartGameLine4)) / 2;
    Show(dx, dy + 3, StartGameLine4);
    dx = (m_columns - strlen(StartGameLine5)) / 2;
    Show(dx, dy + 4, StartGameLine5);
    dx = (m_columns - strlen(StartGameLine6)) / 2;
    Show(dx, dy + 5, StartGameLine6);
    dx = (m_columns - strlen(StartGameLine7)) / 2;
    Show(dx, dy + 6, StartGameLine7);
    dx = (m_columns - strlen(StartGameLine8)) / 2;
    Show(dx, dy + 7, StartGameLine8);
    dx = (m_columns - strlen(StartGameLine9)) / 2;
    Show(dx, dy + 8, StartGameLine9);
    dx = (m_columns - strlen(StartGameLine10)) / 2;
    Show(dx, dy + 11, StartGameLine10);
  }
  void ShowGameOver() {
    static constexpr auto GameOverLine1{
        "   _________    __  _____________ _    ____________ "};
    static constexpr auto GameOverLine2{
        "    / ____/   |  /  |/  / ____/ __ \\ |  / / ____/ __ \\  "};
    static constexpr auto GameOverLine3{
        "     / / __/ /| | / /|_/ / __/ / / / / | / / __/ / /_/ /    "};
    static constexpr auto GameOverLine4{
        "    / /_/ / ___ |/ /  / / /___/ /_/ /| |/ / /___/ _, _/     "};
    static constexpr auto GameOverLine5{
        "  \\____/_/  |_/_/  /_/_____/\\____/ |___/_____/_/ |_|   \n"};
    ClearScreen();
    unsigned short dy = (m_rows - 5) / 2;
    unsigned short dx = (m_columns - strlen(GameOverLine1)) / 2;
    Show(dx, dy, GameOverLine1);
    dx = (m_columns - strlen(GameOverLine2)) / 2;
    Show(dx, dy + 1, GameOverLine2);
    dx = (m_columns - strlen(GameOverLine3)) / 2;
    Show(dx, dy + 2, GameOverLine3);
    dx = (m_columns - strlen(GameOverLine4)) / 2;
    Show(dx, dy + 3, GameOverLine4);
    dx = (m_columns - strlen(GameOverLine5)) / 2;
    Show(dx, dy + 4, GameOverLine5);
  }
};

class SnakeGame {
  struct Apple {
    int dxApple{-1};
    int dyApple{0};
  };

  struct Snake {
    int xDirection{1};
    int yDirection{0};
    int head{0};
    int tail{0};
  };

  static constexpr auto SnakeArraySize{1000};
  static constexpr auto DxOffsetFrame{3};
  static constexpr auto DyOffsetFrame{3};
  static constexpr auto OffsetFrame{2};

  std::array<int, SnakeArraySize> m_dx{0};
  std::array<int, SnakeArraySize> m_dy{0};
  Apple m_apple{};
  Control m_control{};
  Screen m_screen{};
  Snake m_snake{};
  std::uint16_t m_xPlayingFieldMax{0};
  std::uint16_t m_yPlayingFieldMax{0};
  bool m_gameOver{false};
  bool m_quit{false};

  void HideSnakeTail() {
    m_screen.Show(m_dx[m_snake.tail] + OffsetFrame,
                  m_dy[m_snake.tail] + OffsetFrame, " ");
  }

  void ShowApple() {
    if (m_apple.dxApple < 0) {
      m_apple.dxApple = rand() % m_xPlayingFieldMax;
      m_apple.dyApple = rand() % m_yPlayingFieldMax;

      for (int i = m_snake.tail; i != m_snake.head;
           i = (i + 1) % SnakeArraySize) {
        if (m_dx[i] == m_apple.dxApple and m_dy[i] == m_apple.dyApple) {
          m_apple.dxApple = -1;
        }
      }

      if (m_apple.dxApple >= 0) {
        m_screen.Show(m_apple.dxApple + OffsetFrame,
                      m_apple.dyApple + OffsetFrame, "♣");
      }
    }
  }
  void ShowSnakeHead() {
    m_screen.Show(m_dx[m_snake.head] + OffsetFrame,
                  m_dy[m_snake.head] + OffsetFrame, "▓");
    fflush(stdout);
  }

  void GetControlCommand() {
    switch (m_control.ReadKey()) {
    case Control::Escape:
      m_quit = true;
      break;
    case Control::UpArrow:
      m_snake.xDirection = 0;
      m_snake.yDirection = -1;
      break;
    case Control::DownArrow:
      m_snake.xDirection = 0;
      m_snake.yDirection = 1;
      break;
    case Control::LeftArrow:
      m_snake.xDirection = -1;
      m_snake.yDirection = 0;
      break;
    case Control::RightArrow:
      m_snake.xDirection = 1;
      m_snake.yDirection = 0;
      break;
    default:
      break;
    }
  }

public:
  explicit SnakeGame(std::uint16_t xDimension, std::uint16_t yDimension)
      : m_screen(xDimension, yDimension),
        m_xPlayingFieldMax(xDimension - DxOffsetFrame),
        m_yPlayingFieldMax(yDimension - DyOffsetFrame) {
    m_dx[m_snake.head] = m_xPlayingFieldMax / 2;
    m_dy[m_snake.head] = m_yPlayingFieldMax / 2;
  }
  void Welcome() {
    m_screen.ShowStartGame();
    while (not(Control::Enter == m_control.ReadKey())) {
      usleep(5000);
    }
  }
  void Running() {
    std::uint16_t scope{0};
    m_screen.ShowGameField();

    while (not m_quit && not m_gameOver) {
      GetControlCommand();
      ShowApple();
      // MoveSnake(m_control.ReadKey());
      HideSnakeTail();

      if (m_dx[m_snake.head] == m_apple.dxApple and
          m_dy[m_snake.head] == m_apple.dyApple) {
        m_apple.dxApple = -1;
        scope += 10;
        printf("\033[%i;%iH%d", m_yPlayingFieldMax + 3, 9, scope);
      } else {
        m_snake.tail = (m_snake.tail + 1) % SnakeArraySize;
      }

      int newhead = (m_snake.head + 1) % SnakeArraySize;
      m_dx[newhead] =
          (m_dx[m_snake.head] + m_snake.xDirection + m_xPlayingFieldMax) %
          m_xPlayingFieldMax;
      m_dy[newhead] =
          (m_dy[m_snake.head] + m_snake.yDirection + m_yPlayingFieldMax) %
          m_yPlayingFieldMax;
      m_snake.head = newhead;

      for (int i = m_snake.tail; i != m_snake.head;
           i = (i + 1) % SnakeArraySize) {
        if (m_dx[i] == m_dx[m_snake.head] and m_dy[i] == m_dy[m_snake.head]) {
          m_gameOver = true;
        }
      }
      ShowSnakeHead();
      usleep((6 * 1000000 - (500 * scope)) / 60);
    }

    m_screen.ShowGameOver();
  }
};

int main() {
  auto [xDimension, yDimension] = GetTerminalDimensions();
  std::unique_ptr<SnakeGame> game{
      std::make_unique<SnakeGame>(xDimension, yDimension)};
  game->Welcome();
  game->Running();
  return 0;
}
