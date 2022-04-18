#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <Windows.h>
#include <string>

using namespace std;

int main()
{
    ifstream fin("maze.txt");

    int nScreenWidth = 120;
    int nScreenHeight = 40;
    int nMapWidth = 15;
    int nMapHeight = 15;

    float fPlayerX = 1.0f;
    float fPlayerY = 1.0f;
    float fPlayerA = 0.0f;        // направление игрока
    float fFOV = 3.14159f / 4.0f; // угол обзора
    float fDepth = 15.0f;         // макс дистанция обзора
    float fSpeed = 3.0f;

    auto *screen = new wchar_t[nScreenWidth * nScreenHeight];                                                        // Массив для записи в буфер
    auto hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL); // буфер экрана
    SetConsoleActiveScreenBuffer(hConsole);                                                                          // настройка консоли
    DWORD dwBytesWritten = 0;                                                                                        // для дебага
    string maze_string;                                                                                              // это всё метод вывода
    wstring map;                                                                                                     // строковый массив

    while (getline(fin, maze_string))
    {
        wstring maze_wstring(maze_string.begin(), maze_string.end());
        map += maze_wstring;
    }

    auto tp1 = chrono::system_clock::now(); // подсчет пройденного времени
    auto tp2 = chrono::system_clock::now();

    while (1)
    {

        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1; // подсчет пройденного времени после каждой итерации цикла
        tp1 = tp2;                                       // обновление предыдущего времени системы
        float fElapsedTime = elapsedTime.count();

        if (GetAsyncKeyState((unsigned short)'A') & 0x8000) // 0x8000(32768) - возвращает функция, если клавиша нажата
            fPlayerA -= 1.5f * fElapsedTime;

        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += 1.5f * fElapsedTime;

        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime; // вектор, постоянно обновляется
            fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#') // если столкнулись со стеной - откатываем координаты. c_str возвращает указатель на массив, текукщее значение строкового объекта
            {                                                                  // преобразуем в int чтобы работать с двумерным массивом карты
                fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
            {
                fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        for (int x = 0; x < nScreenWidth; x++) //проходим каждый луч на экране
        {
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV; // в первой части делим угол на 2 и находим начальный угол поля зрения, вторая нарезает на мелкие кусочки

            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f; // расстояние от игрока до стены с данным углом

            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle); // векторы, пускающиеся для подсчета расстояния до стены
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth) // или вышли за радиус видимости
            {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall); // пускаем вектор для проверки расстояния до стены
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) // если вышли за пределы карты
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {

                    if (map.c_str()[nTestX * nMapWidth + nTestY] == '#')
                    {

                        bHitWall = true;

                        vector<pair<float, float>> p;

                        for (int tx = 0; tx < 2; tx++)     // создаем грани
                            for (int ty = 0; ty < 2; ty++) // проходим по 4-м ребрам
                            {

                                float vy = (float)nTestY + ty - fPlayerY;        // координаты вектора
                                float vx = (float)nTestX + tx - fPlayerX;        // ведущего изнаблюдателя в ребро
                                float d = sqrt(vx * vx + vy * vy);               // модуль вектора
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // скалярное произведение (единичных векторов)
                                p.push_back(make_pair(d, dot));                  // сохраняем результат в массив
                            }

                        // мы будем выводить два ближайших ребра, поэтому сортируем их по модулю вектора ребра
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right)
                             { return left.first < right.first; });

                        float fBound = 0.01; // угол, при котором начинаем различать ребро.
                        if (acos(p.at(0).second) < fBound)
                            bBoundary = true;
                        if (acos(p.at(1).second) < fBound)
                            bBoundary = true;
                        if (acos(p.at(2).second) < fBound)
                            bBoundary = true;
                    }
                }
            }

            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall); // половина ширины экрана - пропорция экрана относительно расстояния до стены
            int nFloor = nScreenHeight - nCeiling;                                                  // отзкркаленный потолок

            short nShade = ' '; // затемнение стен
            if (fDistanceToWall <= fDepth / 4.0f)
                nShade = 0x2588;
            else if (fDistanceToWall < fDepth / 3.0f)
                nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 2.0f)
                nShade = 0x2592;
            else if (fDistanceToWall < fDepth)
                nShade = 0x2591;
            else
                nShade = ' ';

            if (bBoundary)
                nShade = ' ';

            for (int y = 0; y < nScreenHeight; y++) //рисуем стену и потолок
            {
                if (y <= nCeiling)                      // меньше количества потолков
                    screen[y * nScreenWidth + x] = ' '; // затемняем массив
                else if (y > nCeiling && y <= nFloor)
                    screen[y * nScreenWidth + x] = nShade;
                else
                {

                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));

                    if (b < 0.25) // затемняем пол
                        nShade = '#';
                    else if (b < 0.5)
                        nShade = 'x';
                    else if (b < 0.75)
                        nShade = '.';
                    else if (b < 0.9)
                        nShade = '-';
                    else
                        nShade = ' ';

                    screen[y * nScreenWidth + x] = nShade;
                }
            }
        }

        _snwprintf(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        for (int nx = 0; nx < nMapWidth; nx++)
            for (int ny = 0; ny < nMapWidth; ny++)
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];

        screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

        screen[nScreenWidth * nScreenHeight - 1] = '\0';                                                       // последний символ - окончание строки
        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, {0, 0}, &dwBytesWritten); // запись в буфер, это всё метод вывода
    }

    return 0;
}
