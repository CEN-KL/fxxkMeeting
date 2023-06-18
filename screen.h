#ifndef SCREEN_H
#define SCREEN_H


class Screen
{
public:
    Screen() = default;
    static int width;
    static int height;

    static void init();
};

#endif // SCREEN_H
