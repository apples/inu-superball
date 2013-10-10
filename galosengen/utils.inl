#ifndef UTILS_INL
#define UTILS_INL

class ChainChomp
{
public:
    enum State
    {
        N, T, F
    };

    State state;

    ChainChomp(State s)
        : state(s)
    {}

    template <typename A, typename B>
    ChainChomp& operator()(const A& a, const B& b)
    {
        if (state != N) return *this;
        if      (a<b)  state = T;
        else if (a==b) state = N;
        else           state = F;
        return *this;
    }

    bool get() const
    {
        return (state == T);
    }
};

inline ChainChomp releaseTheChains()
{
    return ChainChomp(ChainChomp::N);
}

template <typename T>
class Ptr
{
    struct Data
    {
        T* ptr;
        int refs;
    } *data;
public:
    Ptr()
        : data(0)
    {}

    Ptr(T* p)
        : data(new Data)
    {
        data->ptr = p;
        data->refs = 1;
    }

    Ptr(const Ptr& in)
        : data(in.data)
    {
        if (data) ++data->refs;
    }

    ~Ptr()
    {
        del();
    }

    Ptr& operator=(const Ptr& in)
    {
        del();
        data = in.data;
        if (data) ++data->refs;
    }

    void del()
    {
        if (data)
        {
            if (--data->refs == 0)
            {
                delete data->ptr;
                delete data;
            }
        }
        data = 0;
    }

    void reset(T* p)
    {
        del();
        data = new Data;
        data->ptr = p;
        data->refs = 1;
    }

    T* operator->() const
    {
        return data->ptr;
    }

    T& operator*() const
    {
        return *(data->ptr);
    }
};

#endif // UTILS_INL
