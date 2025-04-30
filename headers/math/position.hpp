//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_POSITION_HPP
#define CLAMS_POSITION_HPP


class Position
{
public:
    [[nodiscard]] int cx() const { return static_cast<int>(x) >> 4; }

    [[nodiscard]] int cz() const { return static_cast<int>(z) >> 4; }
private:
    double x;
    double y;
    double z;
    float yaw;
    float pitch;
};


#endif //CLAMS_POSITION_HPP
