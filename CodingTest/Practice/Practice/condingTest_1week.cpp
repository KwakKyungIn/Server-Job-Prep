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

//1주차 b번문제
/*
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

*/

//1주차 c번 문제
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int a, b, c;
    vector<int> timeIn(3);
    vector<int> timeOut(3);
    int charge = 0;
    int CarIn = 0;

    cin >> a >> b >> c;
    for (int i = 0; i < 3; i++) {
        cin >> timeIn[i] >> timeOut[i];
    }

    for (int timetick = 0; timetick < 100; timetick++) {
        for (int i = 0; i < 3; i++) {
            if (timeIn[i] == timetick) CarIn++;
            if (timeOut[i] == timetick) CarIn--;
        }

        if (CarIn == 1) charge += a;
        else if (CarIn == 2) charge += 2*b;
        else if (CarIn == 3) charge += 3*c;
    }

    cout << charge << '\n';
    return 0;
}
*/

//1주차 d번 문제
/*
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string s;
    cin >> s;

    int n = (int)s.size();
    for (int i = 0; i < n / 2; ++i) {
        if (s[i] != s[n - 1 - i]) {
            cout << 0 << '\n';
            return 0;
        }
    }
    cout << 1 << '\n';
    return 0;
}
*/

//1주차 e번 문제
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int n = 0;
    int a = 0;
    vector <int> vec(26);
    cin >> n;
    string answer;
    for (int i = 0; i < n; i++) {
        string str="";
        cin >> str;
        for (char c : str) {
            a = (int)(c - 'a');
            vec[a]++;
            break;
        }

    }
    for (int i = 0; i < 26; i++) {
        if (vec[i] >= 5) {
            answer += i + 'a';
        }
    }

    if (answer.length() < 1) cout << "PREDAJA";

    cout << answer;
    
}
*/
//1주차 f번 문제
/*
int main() {
    ios_base::sync_with_stdio(false), cin.tie(NULL); cout.tie(NULL);

    string str;
    getline(cin, str);
    for (int i = 0; i < str.length();i++) {
        if (65 <= (int)str[i] && (int)str[i] <= 77) {
            str[i] = str[i] + 13;
        }
        else if (78 <= (int)str[i] && (int)str[i] <= 90) {
            str[i] = str[i] - 13;
        }
        else if (97 <= (int)str[i] && (int)str[i] <= 109) {
            str[i] = str[i] + 13;
        }
        else if (110 <= (int)str[i] && (int)str[i] <= 122) {
            str[i] = str[i] - 13;
        }
        else {

        }
    }
    cout << str;
}

*/
//1주차 g번 문제
/*
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    cout.tie(NULL);

    string Input;
    int num;
    vector<string> str(100); // 네 틀 유지

    cin >> num;
    cin >> Input;

    for (int i = 0; i < num; i++) {
        string temp;
        cin >> temp;
        str[i] = temp;
    }

    // 패턴 split: prefix*suffix
    size_t star = Input.find('*');
    string prefix = Input.substr(0, star);
    string suffix = Input.substr(star + 1);

    for (int i = 0; i < num; i++) {
        const string& s = str[i];

        // 최소 길이 조건
        if (s.size() < prefix.size() + suffix.size()) {
            cout << "NE\n";
            continue;
        }

        // prefix로 시작?
        if (s.compare(0, prefix.size(), prefix) != 0) {
            cout << "NE\n";
            continue;
        }

        // suffix로 끝?
        if (s.compare(s.size() - suffix.size(), suffix.size(), suffix) != 0) {
            cout << "NE\n";
            continue;
        }

        cout << "DA\n";
    }

    return 0;
}
*/
//1주차 h번 문제
/*
int go(const vector<int>& numbers, int mi) {
    int max1 = 0;
    int sum=0;

    for (int j = 0; j < mi; j++) {
        sum += numbers[j];
    }
    max1 = sum;

    for (int i = 0; i < numbers.size() - mi; i++) {
        
        sum = sum - numbers[i] + numbers[i + mi];
        max1 = max(max1, sum);
       
    }
    return max1;
}

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);

    int num = 0;
    int mi = 0;
    int result;
    cin >> num;
    cin >> mi;

    vector<int> vec(num);

    for (int i = 0; i < num; i++) {
        cin >> vec[i];
    }
    
    result = go(vec, mi);
    cout << result;
    return 0;
}
*/
//1주차 i번 문제
/*
bool isNumber(const string& s) {
    return !s.empty() && isdigit((unsigned char)s[0]);
}

int main(){
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int Nnumber=0;
    int Mnumber=0;
    string temp;
    cin >> Nnumber;//kind
    cin >> Mnumber;//problem

    unordered_map<int, string> m1;
    unordered_map<string, int> m2;

    for (int i = 1; i <= Nnumber; i++) {
        cin >> temp;
        m1[i] = temp;
        m2[temp] = i;
    }
    vector<string> vec(Mnumber);
    for (int i = 0; i < Mnumber; i++) {
        cin >> vec[i];
    }

    for (int i = 0; i < Mnumber; i++) {
        if (isNumber(vec[i])) {
            cout << m1[atoi(vec[i].c_str())]<<'\n';
        }
        else {
            cout << m2[vec[i]] << '\n';;
        }
    }

}
*/
//1주차 j번 문제
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int num = 0;
    int num2 = 0;

    cin >> num;
    vector<int> vec;

    for (int i = 0; i < num; i++) {
        cin >> num2;
        string bufferflush;
        getline(cin, bufferflush);
        int result = 1;
        unordered_map<string, int> m;

        for (int j = 0; j < num2; j++) {
            vector<string> str(num2);


            getline(cin, str[j]);
            string temp;
            size_t p = str[j].find(" ");
            temp = str[j].substr(p+1);
            m[temp]++;
        }

        for (auto& kv : m) {
            result *= kv.second+1;
        }
        vec.push_back(result - 1);

    }
    for (auto i : vec) {
        cout << i<<'\n';
    }
}
*/
//1주차 k번 문제
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    string answer;
    string str;
    int odd = 0;
    map<char, int> m;
    string left, right, middle;
    cin >> str;
    for (char c : str) {
        m[c]++;
    }
    for (auto& kv : m) {
        if (kv.second % 2 != 0) {
            odd++;
        }
    }

    if (odd == 0) {
        middle = "";
        for (auto& kv : m) {
            for (int i = 0; i < kv.second / 2; i++) {
                left += kv.first;
            }
        }
        answer += left;
        reverse(left.begin(), left.end());
        answer += left;
    }
    else if (odd < 2) {
        for (auto& kv : m) {
            if (kv.second % 2 != 0) {
                middle = kv.first;
            }

            for (int i = 0; i < kv.second / 2; i++) {
                left += kv.first;
            }
        }
        answer += left;
        reverse(left.begin(), left.end());
        answer += middle;
        answer += left;
    }
    else {
        cout << "I'm Sorry Hansoo" << '\n';
        return 0;
    }

    cout << answer << '\n';
}
*/
//1주차 l번 문제
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);

    int num1;
    int num2;
    int temp;
    int count=0;
    vector<int> vec(10000000);
    
    cin >> num1;
    cin >> num2;
    vector<int> number(num1);
    for (int i = 0; i < num1; i++) {
        cin >> temp;
        number[i] = temp;
    }
    for (int i = 0; i < num1 - 1; i++) {
        for (int j = i+1; j < num1; j++) {
            vec[number[i] + number[j]]++;
        }
    }

    cout << vec[num2];

}
*/
//1주차 n번 문제
/*
bool crushPairsEmpty(const string& s) {
    vector<pair<char, int>> st;
    st.reserve(s.size());

    for (char c : s) {
        if (!st.empty() && st.back().first == c) st.back().second++;
        else st.push_back({ c, 1 });

        if (st.back().second == 2) st.pop_back();
    }

    return st.empty(); // 최종 문자열이 빈 문자열이면 true
}

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int num = 0;
    int result = 0;

    cin >> num;
    vector<string> str(num);
    for (int i = 0; i < num; i++) {
        cin >> str[i];
        if (crushPairsEmpty(str[i])) result++;
    }
    
    cout << result;


}
*/

//1주차 m번 문제
//1주차 o번 문제
