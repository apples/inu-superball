#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>

using namespace std;

class Enumerator
{
public:
    Enumerator (int n, int m)
        : data(n)
        , mx(m)
    {
        for (int i=0; i<n; ++i) data[i] = i;
    }
    
    bool next()
    {
        return inc(data.size()-1);
    }
    
    bool inc(int i)
    {
        if (++data[i] >= mx-(data.size()-i-1))
        {
            if (i==0) return false;
            data[i]  = data[i-1]+2;
            return inc(i-1);
        }
        return true;
    }
    
    vector<int> data;
    int mx;
};

int main(int argc, char* argv[])
{
    if (argc != 2) return -1;
    
    const int num = atoi(argv[1]);
    
    if (num < 1) return -2;
    
    for (int w=1; w<=num; ++w)
    {
        Enumerator en(w, num);
        do
        {
            vector<int> nums = en.data;
            do
            {
                for (int i=0; i<w; ++i)
                {
                    if (i!=0) cout << " ";
                    cout << nums[i];
                }
                cout << endl;
            } while (next_permutation(nums.begin(), nums.end()));
        } while (en.next());
    }
}
