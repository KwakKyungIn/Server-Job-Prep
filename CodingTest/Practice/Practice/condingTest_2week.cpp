#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <cmath>
#include <limits>
#include <memory>
using namespace std;

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    vector<int> v(26);
    int temp = 0;
    string s;
    cin >> s;
    for (auto c : s) {
        temp =(int)( c - 'a');
        v[temp]++;
    }
    for (int i : v) {
        cout << i << ' ';
    }


    return 0;
}
