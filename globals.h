#pragma once

struct negative_one {
    template<class T>
    operator T() const
    {
        return static_cast<T>(~T());
    }
};

extern clock_t const begin_time;
extern struct negative_one negative_one;

