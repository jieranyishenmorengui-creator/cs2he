#pragma once
enum bones : int
{
    head = 6,
    neck_0 = 5,
    spine_1 = 4,
    spine_2 = 2,
    cock = 0,
    arm_upper_L = 8,
    arm_lower_L = 9,
    hand_L = 10,
    arm_upper_R = 13,
    arm_lower_R = 14,
    hand_R = 15,
    leg_upper_L = 22,
    leg_lower_L = 23,
    ankle_L = 24,
    leg_upper_R = 25,
    leg_lower_R = 26,
    ankle_R = 27,
};

struct BoneConnection {
    int from;
    int to;
};


const std::pair<int, int> bone_connections[] = {
    {6, 34}, 
    {6, 33},

    {6, 5},
    {5, 4},
    {4, 0},
    {4, 8},
    {8, 9},
    {9, 11},
    {4, 13},
    {13, 14},
    {14, 16},
    {4, 2},
    {0, 22},
    {0, 25},
    {22, 23},
    {23, 24},
    {25, 26},
    {26, 27}
};


