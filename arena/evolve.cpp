
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <string>
#include <fstream>
#include <utility>
#include "libexecstream/exec-stream.h"

using namespace std;

struct Gene
{
    vector<int> val;
    int high;
    float avg;
};

bool compAvg(const Gene& a, const Gene& b)
{
    return (a.avg>b.avg);
}

bool compHigh(const Gene& a, const Gene& b)
{
    return (a.high>b.high);
}

class EnumerateSubgenes
{
public:
    vector<int> data;
    int pos;
    int len;
    
    EnumerateSubgenes(vector<int> v)
        : data(move(v))
        , pos(0)
        , len(data.size())
    {}
    
    bool match(const vector<int>& other)
    {
        for (int i=0; i<other.size()-len+1; ++i)
        {
            for (int j=0; j<len; ++j)
            {
                if (other[i+j] != data[pos+j]) break;
                if (j == len-1) return true;
            }
        }
        return false;
    }
    
    bool next()
    {
        ++pos;
        if (pos+len > data.size())
        {
            pos = 0;
            --len;
            if (len == 1) return false;
        }
        return true;
    }
    
    bool hardnext()
    {
        ++pos;
        if (pos+len > data.size())
        {
            return false;
        }
        return true;
    }
    
    vector<int>::const_iterator begin() const
    {
        return data.begin()+pos;
    }
    
    vector<int>::const_iterator end() const
    {
        return data.begin()+pos+len;
    }
};

class Enumerator
{
public:
    Enumerator (int n)
        : data(n)
    {
        for (int i=0; i<n; ++i) data[i] = i;
    }
    
    bool next()
    {
        return inc(data.size()-1);
    }
    
    bool inc(int i)
    {
        if (++data[i] >= 10-(data.size()-i-1))
        {
            if (i==0) return false;
            data[i]  = data[i-1]+2;
            return inc(i-1);
        }
        return true;
    }
    
    vector<int> data;
};

int main(int argc, char* argv[]) try
{
    int run = 0;
    
    Gene day1;
    day1.high = 0;
    day1.avg = 0.f;
    
    mt19937 rng(time(nullptr));
    
    while (true)
    {
        vector<Gene> generation;
        Enumerator en(3);
        do
        {
            vector<int> seq = en.data;
            do
            {
                {
                    bool skipIt = false;
                    cout << "Doing";
                    for (int i : seq)
                    {
                        cout << " " << i;
                        if (i<0 || i>=10) skipIt = true;
                    }
                    if (skipIt) cout << " (SKIP)";
                    cout << endl;
                    if (skipIt) continue;
                }
                
                Gene g = day1;
                g.val = seq;
                
                ofstream aifile("galo-normal.txt");
                
                for (int j=0; j<g.val.size(); ++j)
                {
                    if (j!=0) aifile << " ";
                    aifile << g.val[j];
                }
                
                auto doRun = [&]
                {
                    float rvalAvg;
                    int rvalHigh;
                    
                    exec_stream_t process;
                    process.set_wait_timeout(exec_stream_t::s_all, 1000*60*5);
                    process.start("./arena", "5");
                    
                    string line;
                    getline(process.out(), line);
                    
                    auto cpos = line.find(':');
                    
                    stringstream ss(line.substr(cpos+1));
                    ss >> rvalAvg >> rvalHigh;
                    
                    return make_pair(rvalAvg, rvalHigh);
                };
                
                auto p1 = doRun();
                auto p2 = doRun();
                
                g.avg = (p1.first+p2.first)/2.f;
                g.high = max(p1.second, p2.second);
                
                cout << "    Avg: " << g.avg << "; High: " << g.high << endl;
                
                generation.push_back(g);
            } while (next_permutation(seq.begin(), seq.end()));
        } while (en.next());
        
        cout << "Parsing results..." << endl;
        
        sort(generation.begin(), generation.end(), compAvg);
        sort(generation.begin()+10, generation.end(), compHigh);
        generation.erase(generation.begin()+20, generation.end());
        
        EnumerateSubgenes matcher(generation[0].val);
        
        auto countMatch = [&]
        {
            int rval = 1;
            for (int i=1; i<generation.size(); ++i)
            {
                if (matcher.match(generation[i].val))
                {
                    ++rval;
                }
            }
            return rval;
        };
        
        vector<pair<int, vector<int>>> subgenes;
        
        do
        {
            subgenes.emplace_back(countMatch(), vector<int>(matcher.begin(), matcher.end()));
        } while (matcher.next());
        
        auto sorter = [](const pair<int, vector<int>>& a, const pair<int, vector<int>>& b)
        {
            return (a.first>b.first);
        };
        
        sort(subgenes.begin(), subgenes.end(), sorter);
        
        {
            auto iter = subgenes.begin();
            auto a1 = iter->first;
            while (iter != subgenes.end() && a1-iter->first<3) ++iter;
            subgenes.erase(iter, subgenes.end());
        }
        
        stringstream ss;
        ss << "results" << run << ".txt";
        
        ofstream resfile(ss.str().c_str());
        
        for (int i=0; i<generation.size(); ++i)
        {
            resfile << "Gene " << i << ":";
            for (int j=0; j<generation[i].val.size(); ++j)
            {
                resfile << " " << generation[i].val[j];
            }
            resfile << endl;
            resfile << "    Score: " << generation[i].avg << endl;
            resfile << "    Max:   " << generation[i].high << endl;
        }
        
        resfile << "Common subgenes:" << endl;
        
        for (auto&& p : subgenes)
        {
            auto&& vec = p.second;
            resfile << p.first << " :";
            for (int i=0; i<vec.size(); ++i)
            {
                resfile << " " << vec[i];
            }
            resfile << endl;
        }
        
        break;
    }
}
catch (std::exception const& e)
{
    cerr << "Error: " << e.what() << endl;
    return -1;
}
