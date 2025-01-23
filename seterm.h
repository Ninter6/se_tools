#pragma once

#include <cstdio>
#include <string>
#include <iostream>

#ifdef _WIN32
#define ST_WIN
#include <windows.h>
#include <conio.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#define ST_UNIX
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#else
#error "Unknow platform!"
#endif

namespace st {

struct NonBlockIO {
#ifdef ST_UNIX
    NonBlockIO() {
        struct termios new_settings{};
        tcgetattr(0,&stored_settings);
        new_settings = stored_settings;
        new_settings.c_lflag &= (~ICANON);
        new_settings.c_cc[VTIME] = 0;
        new_settings.c_cc[VMIN] = 1;
        tcsetattr(0,TCSANOW,&new_settings);

        int att = 1;
        ioctl(0, FIONBIO, &att);
    }
    ~NonBlockIO() {
        int att = 0;
        ioctl(0, FIONBIO, &att);

        tcsetattr(0,TCSANOW,&stored_settings);
    }
    NonBlockIO(NonBlockIO&&) = delete;

    struct termios stored_settings{};
#endif
};

inline int getKey() {
#ifdef ST_UNIX
    NonBlockIO nb{};
    return getchar();
#elif defined(ST_WIN)
    if (_kbhit()) return _getch();
    else return -1;
#endif
}

inline std::string readline(std::string_view title) {
    std::string buf;
    std::cout << title;
#if ST_WIN
    return std::getline(cin, buf);
#else
    auto nb = new NonBlockIO;
    int c, i=0;
    [&]{while (true) {
        while ((c = getchar()) == -1);
        switch (c) {
            case '\e':
                if (getchar() == '[')
                    switch (getchar()) {
                        case 'A': i=0; break;
                        case 'B': i=(int)buf.size(); break;
                        case 'C': i=std::min(i+1, (int)buf.size()); break;
                        case 'D': i=std::max(i-1, 0); break;
                        case '3': if (getchar() == '~') buf.erase(i, 1); break;
                    }
                break;
            case 127:
                if (i>0) buf.erase(--i, 1);
                break;
            case '\n':
                return;
            default:
                buf.insert(buf.begin() + i++, (char)c);
                break;
        }
        std::cout << "\r\e[K" << title << buf;
        for (int j = 0; j < buf.size() - i; j++)
            std::cout.put('\b');
    }}();
    delete nb;
    return buf;
#endif
}

}
