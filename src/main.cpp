#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>
#include <windows.h>

enum class SpecialKeys
{
    KEY_UP = 72,
    KEY_DOWN = 80,
    KEY_LEFT = 75,
    KEY_RIGHT = 77,
    BACKSPACE = 8
};

enum class ColorText
{
    RESET = 0,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_BLUE = 94,
    BRIGHT_WHITE = 97
};

// colorising the stdout with specific color
std::ostream& operator<<(std::ostream& os, ColorText color)
{
    return os << "\033[" << static_cast<int>(color) << "m";
}

using SudokuBoard = std::vector<std::vector<std::pair<char, ColorText>>>;

// checks if index of board is on spot of grid char
bool isGridIndex(size_t index, size_t dimentionSize)
{
    return index != 0 && index != dimentionSize - 1 && (index + 1) % 3 == 0;
}

void printSudokuBoard(const SudokuBoard& board)
{
    size_t columnSize = board.size();
    size_t rowSize = board[0].size();
    std::string gridDelim = "| ";
    size_t horisontalGridLength = rowSize + rowSize - 1 + gridDelim.size() * 2;

    std::cout << "\033[0;0H\033[K"; // move cursor to pos {0, 0}
    std::cout.flush();              // reset stdout buffer

    for (size_t i = 0; i < columnSize; i++)
    {
        for (size_t j = 0; j < rowSize; j++)
        {
            std::cout << board[i][j].second << board[i][j].first << ' ' << ColorText::RESET;

            if (isGridIndex(j, rowSize))
            {
                std::cout << "| ";
            }
        }

        if (isGridIndex(i, columnSize))
        {
            std::cout << std::endl;
            

            for (int k = 0; k < horisontalGridLength; k++)
            {
                char rowGrid = '-';

                if (k == 6 || k == 14)
                {
                    rowGrid = '+';
                }

                std::cout << rowGrid;
            }
        }

        std::cout << std::endl;
    }
}

HANDLE cursorHandle = GetStdHandle(STD_OUTPUT_HANDLE);

void setCursorPosition(int x, int y)
{
    COORD coordinates{};
    coordinates.X = x;
    coordinates.Y = y;
    SetConsoleCursorPosition(cursorHandle, coordinates);
}

void showConsoleCursor(bool showFlag)
{
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(cursorHandle, &cursorInfo);
    cursorInfo.bVisible = showFlag;
    SetConsoleCursorInfo(cursorHandle, &cursorInfo);
}

char cursorCharRead()
{
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbiInfo);
    COORD pos = csbiInfo.dwCursorPosition;
    TCHAR strFromConsole[1];
    DWORD dwChars;
    ReadConsoleOutputCharacter(hConsole, strFromConsole, 1, pos, &dwChars);
    return static_cast<char>(strFromConsole[0]);
}

inline int getMatrixRow(int index)
{
    int pureIndex = index;
    int quartOfIndex = index / 4;

    if (quartOfIndex == 1)
    {
        pureIndex = index - 1;
    }
    else if (quartOfIndex == 2)
    {
        pureIndex = index - 2;
    }

    return pureIndex;
}

inline int getMatrixCol(int pos)
{
    int pureIndex = pos / 2;

    if (pureIndex == 10)
    {
        return 8;
    }

    if (pureIndex >= 3)
    {
        pureIndex -= pureIndex % 3 == 0 ? pureIndex / 3 - 1 : pureIndex / 3;
    }

    return pureIndex;
}

std::pair<int, int> getMatrixPos(const std::pair<int, int>& pos)
{
    int rowIndex = getMatrixRow(pos.second);
    int colIndex = getMatrixCol(pos.first);
    return std::make_pair(rowIndex, colIndex);
}

std::vector<std::vector<int>> initialBoardSample = {
    { {5, 3, 0, 0, 7, 0, 0, 0, 0} },
    { {6, 0, 0, 1, 9, 5, 0, 0, 0} },
    { {0, 9, 8, 0, 0, 0, 0, 6, 0} },
    { {8, 0, 0, 0, 6, 0, 0, 0, 3} },
    { {4, 0, 0, 8, 0, 3, 0, 0, 1} },
    { {7, 0, 0, 0, 2, 0, 0, 0, 6} },
    { {0, 6, 0, 0, 0, 0, 2, 8, 0} },
    { {0, 0, 0, 4, 1, 9, 0, 0, 5} },
    { {0, 0, 0, 0, 8, 0, 0, 7, 9} }
};

SudokuBoard getSudokuBoard()
{
    SudokuBoard board(9, std::vector<std::pair<char, ColorText>>(9, std::make_pair('.', ColorText::BRIGHT_WHITE)));

    for (size_t i = 0; i < initialBoardSample.size(); i++)
    {
        for (size_t j = 0; j < initialBoardSample[0].size(); j++)
        {
            if (initialBoardSample[i][j] != 0)
            {
                board[i][j] = std::make_pair(initialBoardSample[i][j] + '0', ColorText::BRIGHT_GREEN);
            }
        }
    }

    return board;
}

inline bool checkCrossPaths(const SudokuBoard& board, int row, int col)
{
    char target = board[row][col].first;
    for (size_t i = 0; i < board.size(); i++)
    {
        if (target == board[i][col].first && i != row)
        {
            return false;
        }

        if (target == board[row][i].first && i != col)
        {
            return false;
        }
    }

    return true;
}

inline bool checkSubMatrix(const SudokuBoard& board, int row, int col)
{
    char target = board[row][col].first;

    int startIndexRow = row / 3 * 3;
    int startIndexColumn = col / 3 * 3;

    for (int i = startIndexRow; i < startIndexRow + 3; i++)
    {
        for (int j = startIndexColumn; j < startIndexColumn + 3; j++)
        {
            if (i == row && j == col)
            {
                continue;
            }

            if (board[i][j].first == target)
            {
                return false;
            }
        }
    }

    return true;
}

void keyboardGameHandle(int delay)
{
    SudokuBoard board = getSudokuBoard();
    std::pair<int, int> currentPosition{ 0, 0 };

    while (true)
    {
        showConsoleCursor(false);
        printSudokuBoard(board);
        setCursorPosition(currentPosition.first, currentPosition.second);
        showConsoleCursor(true);

        if (_kbhit())
        {
            int inputKey = _getch();

            if (inputKey == 224)
            {
                inputKey = _getch();
                SpecialKeys enumKey = static_cast<SpecialKeys>(inputKey);

                switch (enumKey)
                {
                case SpecialKeys::KEY_UP:
                    setCursorPosition(currentPosition.first, --currentPosition.second);
                    break;
                case SpecialKeys::KEY_DOWN:
                    setCursorPosition(currentPosition.first, ++currentPosition.second);
                    break;
                case SpecialKeys::KEY_LEFT:
                    setCursorPosition(--currentPosition.first, currentPosition.second);
                    break;
                case SpecialKeys::KEY_RIGHT:
                    setCursorPosition(++currentPosition.first, currentPosition.second);
                    break;
                }
            }
            else if (SpecialKeys::BACKSPACE == static_cast<SpecialKeys>(inputKey))
            {
                if (cursorCharRead() != ' ')
                {
                    std::pair<int, int> pos = getMatrixPos(currentPosition);
                    auto& element = board[pos.first][pos.second];
                    if (element.second != ColorText::BRIGHT_GREEN)
                    {
                        element = std::make_pair('.', ColorText::BRIGHT_WHITE);
                    }
                }
            }
            else
            {
                char key = static_cast<int>(inputKey);
                if (cursorCharRead() != ' ' && std::isdigit(key) && key != '0')
                {
                    std::pair<int, int> pos = getMatrixPos(currentPosition);
                    auto& element = board[pos.first][pos.second];
                    if (element.second != ColorText::BRIGHT_GREEN)
                    {
                        element.first = inputKey;
                        bool isValidDigit = checkCrossPaths(board, pos.first, pos.second) && checkSubMatrix(board, pos.first, pos.second);
                        element.second = isValidDigit ? ColorText::BRIGHT_BLUE : ColorText::BRIGHT_RED;
                    }
                }
            }            
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{ delay });        
    }
}

int main()
{
    keyboardGameHandle(20);
    return 0;
}
