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
#include <iomanip>
using namespace std;
//a번 문제
/*
const int dx[] = { -1, 1, 0, 0 };
const int dy[] = { 0, 0, -1, 1 };

void go(vector<vector<int>>& grid, vector<vector<int>> &visited, int row, int col, int sy, int sx) {

    visited[sy][sx] = 1;

    for (int i = 0; i < 4; i++) {
        int nx = sx + dx[i];
        int ny = sy + dy[i];
        if (ny < 0 || ny >= row || nx < 0 || nx >= col) continue;
        if (grid[ny][nx] == 0) continue;
        if (grid[ny][nx] == 1 && !visited[ny][nx]) {
            go(grid, visited, row, col, ny, nx);
        }

    }
}

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);
    int testCase = 0;
    int row = 0;
    int col = 0;
    int count = 0;
    int answer1 = 0;
    

    cin >> testCase;

    vector<int> answer(testCase);
    for (int i = 0; i < testCase; i++) {

        cin >> col >> row >> count;   // M(가로)=col, N(세로)=row

        int answer1 = 0;              // 테스트케이스마다 초기화

        vector<vector<int>> grid(row, vector<int>(col));
        vector<vector<int>> visited(row, vector<int>(col));

        for (int j = 0; j < count; j++) {
            int x, y;
            cin >> x >> y;            // 입력은 (x, y)
            grid[y][x] = 1;           // grid[y][x]
        }

        for (int y = 0; y < row; y++) {
            for (int x = 0; x < col; x++) {
                if (grid[y][x] == 1 && visited[y][x] == 0) {
                    answer1++;
                    go(grid, visited, row, col, y, x);
                }
            }
        }

        answer[i] = answer1;
    }

    for (auto i : answer) cout << i<<'\n';

    return 0;
}

*/

//b번 문제
/*

const int dy[]={ -1, 1, 0 ,0 };
const int dx[] = { 0,0,-1,1 };
void dfs(vector<vector<int>>& grid, vector<vector<int>>& visited, int n, int sx, int sy, int height) {
    visited[sy][sx] = 1;
    for (int i = 0; i < 4; i++) {
        int nx = sx + dx[i];
        int ny = sy + dy[i];

        if (nx < 0 || nx >= n || ny < 0 || ny >= n) continue;
        if (grid[ny][nx] <= height) continue;
        if (grid[ny][nx] > height && !visited[ny][nx]) {
            dfs(grid, visited, n, nx, ny, height);
        }
    }
}



int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);
    int n = 0;
    int maxH = 0;
    int answer = 0;
    cin >> n;
    vector<vector<int>> grid(n, vector<int>(n));
    vector<vector<int>> visited(n, vector<int>(n));
    vector<vector<int>> visited_new(n, vector<int>(n));

    for (int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            cin >> grid[i][j];
            maxH = max(maxH, grid[i][j]);
        }
    }

    for (int x = 0; x < maxH; x++) {
        int result = 0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (grid[i][j] > x && visited[i][j] == 0) {
                    result++;
                    dfs(grid, visited, n, j, i, x);
                }
            }
        }
        answer = max(answer, result);
        visited = visited_new;
    }

    cout << answer;


}*/
//c
/*
const int dx[] = { -1, 1, 0, 0 };
const int dy[] = { 0, 0, -1, 1 };
int count2 = 0;

void go(vector<vector<int>>& grid, vector<vector<int>> &visited, int row, int col, int sy, int sx) {
    visited[sy][sx] = 1;
    count2++;
    for (int i = 0; i < 4; i++) {
        int nx = sx + dx[i];
        int ny = sy + dy[i];
        if (ny < 0 || ny >= row || nx < 0 || nx >= col) continue;
        if (grid[ny][nx] == 1) continue;
        if (grid[ny][nx] == 0 && !visited[ny][nx]) {
            go(grid, visited, row, col, ny, nx);
        }

    }
}

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); cout.tie(NULL);

    int ty = 0;
    int tx = 0;
    int count1 = 0;

    int area = 0;

    cin >> ty >> tx >> count1;
    vector<vector<int>> grid(ty, vector<int>(tx));
    vector<vector<int>> visited(ty, vector<int>(tx));

    for (int i = 0; i < count1; i++) {
        int minX = 0 , minY=0, maxX=0, maxY = 0;
        cin >> minX >> minY >> maxX >> maxY;

        for (int a = minX; a < maxX; a++) {
            for (int b = minY; b< maxY; b++) {
                grid[b][a] = 1;
            }
        }

    }

    vector<int> answer;

    for (int y = 0; y < ty; y++) {
        for (int x = 0; x < tx; x++) {
            if (grid[y][x] == 0 && visited[y][x] == 0) {
                area++;
                go(grid, visited, ty,tx , y, x);
                answer.push_back(count2);
                count2 = 0;
            }
        }
    }

    sort(answer.begin(), answer.end());

    cout << area << '\n';
    for (int i : answer) cout << i << ' ';

    return 0;
}

*/
//d
/*
vector<string> grid;   // '0'/'1' 문자로 들고 있으면 편함
string ret;

bool isUniform(int sy, int sx, int size) {
    char first = grid[sy][sx];
    for (int y = sy; y < sy + size; y++) {
        for (int x = sx; x < sx + size; x++) {
            if (grid[y][x] != first) return false;
        }
    }
    return true;
}

void go(int sy, int sx, int size) {
    if (isUniform(sy, sx, size)) {
        ret.push_back(grid[sy][sx]);   // '0' or '1'
        return;
    }

    ret.push_back('(');
    int half = size / 2;

    go(sy, sx, half);  // TL
    go(sy, sx + half, half);  // TR
    go(sy + half, sx, half);  // BL
    go(sy + half, sx + half, half);  // BR

    ret.push_back(')');
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    grid.resize(n);
    for (int i = 0; i < n; i++) cin >> grid[i];

    go(0, 0, n);
    cout << ret;
}
*/
//e
/*
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int vSize;
    int bSize;
    int aCount;
    int ret = 0;

    cin >> vSize >> bSize;
    cin >> aCount;

    int left = 1;
    int right = bSize;

    for (int i = 0; i < aCount; i++) {
        int pos;
        cin >> pos;

        if (pos < left) {
            ret += (left - pos);
            left = pos;
            right = left + bSize - 1;
        }
        else if (pos > right) {
            ret += (pos - right);
            left += (pos - right);
            right = left + bSize - 1;
        }
    }

    cout << ret;
    return 0;
}
*/
//f
/*
struct NodeInfo {
    int order;       // 나온 순서(처음 등장 인덱스)
    long long value; // 값
    int freq;        // 빈도
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, dummy;
    cin >> n >> dummy; // 두 번째 값은 일단 무시

    vector<long long> input(n);
    vector<NodeInfo> infos;

    // 입력 받으면서 (값, 첫등장순서, 빈도) 채우기
    for (int i = 0; i < n; i++) {
        long long x;
        cin >> x;
        input[i] = x;

        bool found = false;
        for (auto& it : infos) {
            if (it.value == x) {
                it.freq++;
                found = true;
                break;
            }
        }
        if (!found) {
            infos.push_back(NodeInfo{ i, x, 1 });
        }
    }

    // 빈도 내림차순, 빈도 같으면 order 오름차순
    sort(infos.begin(), infos.end(), [](const NodeInfo& a, const NodeInfo& b) {
        if (a.freq != b.freq) return a.freq > b.freq;
        return a.order < b.order;
        });

    // infos 기준으로 input 재구성
    input.clear();
    input.reserve(n);

    for (const auto& it : infos) {
        for (int k = 0; k < it.freq; k++) {
            input.push_back(it.value);
        }
    }

    for (auto x : input) cout << x << ' ';

    return 0;
}
*/
//g
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);
    string temp = "";
    bool ret=0;
    char pre_word = ' ';
    int count_a = 0;
    int count_b = 0;
    vector<string> str;
    do {
        cin >> temp;
        str.push_back(temp);

    } while (temp != "end");
    for (string s : str) {
        if (s == "end") return 0;

        for (char c : s) {
            if (c == 'a' || c == 'o' || c == 'i' || c == 'u' || c == 'e') {
                ret = 1;
            }
        }

        for (char c : s) {
           

            if (c == 'a' || c == 'o' || c == 'i' || c == 'u' || c == 'e') {
                count_a++;
                count_b = 0;
            }
            else {
                count_b++;
                count_a = 0;
            }
            if (count_a == 3 || count_b == 3) {
                ret = 0;
            }

            if (pre_word == ' ') {
                pre_word = c;
                continue;
            }

            if (pre_word == c) {
                if (c == 'e' || c == 'o') {
                    continue;
                }
                ret = 0;
            }
            pre_word = c;

            
        }
        count_a = 0;
        count_b = 0;
        pre_word = ' ';


        if(ret == true) cout << '<' << s << '>' << " is acceptable." << '\n';
        else {
            cout << '<' << s << '>' << " is not acceptable." << '\n';
        }
        ret = 0;
    }
    
}

*/
//h
/*
static string Normalize(string s) {
    // s는 " 009" 같이 앞에 공백이 붙어온 형태라서 공백 제거 먼저
    if (!s.empty() && s[0] == ' ') s.erase(s.begin());

    // 앞의 0 제거
    int idx = 0;
    while (idx < (int)s.size() && s[idx] == '0') idx++;

    if (idx == (int)s.size()) return "0";   // "000" -> "0"
    return s.substr(idx);                   // "009" -> "9"
}

static bool Cmp(const string& a, const string& b) {
    if (a.size() != b.size()) return a.size() < b.size();
    return a < b;
}

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);

    int line = 0;
    string str;
    string str2 = " ";
    cin >> line;

    vector<string> ret;

    for (int i = 0; i < line; i++) {
        cin >> str;
        for (char c : str) {
            if (c < '0' || c > '9') {
                if (str2 == " ") continue;
                ret.push_back(Normalize(str2));
                str2 = " ";
            }
            else {
                str2 += c;
            }
        }
        if (str2 != " ") {
            ret.push_back(Normalize(str2));
            str2 = " ";
        }
    }

    sort(ret.begin(), ret.end(), Cmp);
    for (const string& s : ret) cout << s << '\n';
}

*/
//i
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);

    int w, h;
    char temp;
    int pre_num = -1;
    cin >> h >> w;

    vector<vector<int>> grid(h,vector<int>(w,-1));

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            cin >> temp;
            if (temp == 'c') {
                grid[i][j] = 0;
            }
        }
    }

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (grid[i][j] == 0) {
                pre_num = 0;
            }
            else if(grid[i][j]== -1) {
                if (pre_num != -1) {
                    grid[i][j] = 0;
                    pre_num += 1;
                    grid[i][j] += pre_num;
                }
                else {
                    pre_num = -1;
                }
            }
        }
        pre_num = -1;
    }

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            cout << grid[i][j] << (j + 1 == w ? '\n' : ' ');
        }
    }

    



}
*/
//j
/*
vector<string> split(const string& input, const string& delimiter) {
    vector<string> result;
    size_t start = 0;
    size_t end = input.find(delimiter);

    while (end != string::npos) {
        result.push_back(input.substr(start, end - start));
        start = end + delimiter.size();
        end = input.find(delimiter, start);
    }

    result.push_back(input.substr(start));
    return result;
}
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int a_score = 0;
    int b_score = 0;
    int total_goal = 0;
    int last_goal_time = 0;
    int a_time = 0;
    int b_time = 0;

    cin >> total_goal;

    vector<string> time(total_goal);
    for (int i = 0; i < total_goal; i++) {

        int temp = 0;
        cin >> temp >> time[i];

        vector<string> goal_time = split(time[i], ":");
        int h = atoi(goal_time[0].c_str());
        int m = atoi(goal_time[1].c_str());
        int temp_time = h * 60 + m;

        if (a_score < b_score) {
            b_time += temp_time - last_goal_time;
        }
        else if (a_score > b_score) {
            a_time += temp_time - last_goal_time;
        }

        if (temp == 1) {
            a_score++;
        }
        else if (temp == 2) {
            b_score++;
        }

        last_goal_time = h * 60 + m;
    }

    if (a_score < b_score) {
        b_time += 2880 - last_goal_time;
    }
    else if (a_score > b_score) {
        a_time += 2880 - last_goal_time;
    }

    cout << setw(2) << setfill('0') << a_time / 60 << ':' << setw(2) << setfill('0') << a_time % 60 << "\n";
    cout << setw(2) << setfill('0') << b_time / 60 << ':' << setw(2) << setfill('0') << b_time % 60 << "\n";
   
}

*/
//k
/*
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL); 

    int LARGE = 1e7;
    int cnt = 0;
    int number = 0;

    cin >> number;

    for (int i = 0; i < LARGE; i++) {
        string s = to_string(i);
        if (s.find("666") != string::npos) {
            cnt++;
        }

        if (cnt == number) {
            cout << i << "\n";
            break;
        }
    }
}
*/
//l
/*
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    cin >> n;

    while (n--) {
        string str;
        cin >> str;

        stack<char> s;
        bool ok = true;

        for (char c : str) {
            if (c == '(') {
                s.push(c);
            }
            else {
                if (s.empty()) {
                    ok = false;
                    break;
                }
                s.pop();
            }
        }

        if (!s.empty()) ok = false;

        if (ok) cout << "YES\n";
        else cout << "NO\n";
    }
}
*/
//n
/*

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);

    while (1) {
        string str;
        bool answer = 1;
        stack<char> st1;
        stack<char> st2;

        getline(cin, str);
        if (str == ".") break;

        for (char c : str) {
            if (c == '(') {
                st1.push(c);
            }
            else if (c == '[') {
                st1.push(c);
            }
            else if (c == ')') {
                if (st1.empty()) {
                    answer = 0;
                    break;
                }
                if (st1.top() == '[') {
                    answer = 0;
                    break;
                }

                st1.pop();
            }
            else if( c== ']') {
                if (st1.empty()) {
                    answer = 0;
                    break;
                }

                if (st1.top() == '(') {
                    answer = 0;
                    break;
                }
                st1.pop();
            }
        }

        if (!st1.empty()) answer = 0;

        if (answer) cout << "yes" << "\n";
        if (!answer) cout << "no" << "\n";
    }
}

*/


/*
const int dy[] = { -1, 1, 0 ,0 };
const int dx[] = { 0,0,-1,1 };
vector<pair<int, int>> empty_grid;
vector<pair<int, int>> virus;

void dfs(vector<vector<int>> &grid, vector<vector<int>> &visited,int mx, int my, int sx, int sy) {
    visited[sy][sx] = 1;
    for (int i = 0; i < 4; i++) {
        int nx = sx + dx[i];
        int ny = sy + dy[i];

        if (nx < 0 || nx >= mx || ny < 0 || ny >= my ) continue;
        if (visited[ny][nx] == 1) continue;
        if (grid[ny][nx]==0) {
            grid[ny][nx] = 2;
            dfs(grid, visited, mx,my, nx, ny);
        }
    }
}



int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);
    int col = 0;
    int row = 0;
    int cnt = 0;
    cin >> col>>row;

    vector<vector<int>> grid(col, vector<int>(row));
    vector<vector<int>> grid2(col, vector<int>(row));
    vector<vector<int>> visited(col, vector<int>(row));

    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            cin >> grid[i][j];
            if (grid[i][j] == 0) {
                empty_grid.push_back({ i,j});
            }
            if (grid[i][j] == 2) {
                virus.push_back({ i,j });
            }
        }
    }
    grid2 = grid;
    size_t max1 = empty_grid.size();

    for (int i = 0; i < max1; i++) {
        for (int j = i+1; j < max1; j++) {
            for (int k = j+1; k < max1; k++) {
                int temp = 0;
                grid[empty_grid[i].first][empty_grid[i].second] =1 ;
                grid[empty_grid[j].first][empty_grid[j].second] = 1;
                grid[empty_grid[k].first][empty_grid[k].second]= 1;

                for (auto a : virus) {
                    dfs(grid, visited, row, col, a.second, a.first);
                }

                for (int i = 0; i < col; i++) {
                    for (int j = 0; j < row; j++) {
                        if (grid[i][j] == 0) {
                            temp++;
                        }

                    }
                }
                cnt=max(cnt, temp);


                visited.assign(col, vector<int>(row, 0));
                grid = grid2;

            }
        }
    }
    cout << cnt;
}
*/


/*
const int dy[] = { -1, 1, 0, 0 };
const int dx[] = { 0, 0, -1, 1 };

int cnt = 0;   // 이번 턴에 녹은 치즈 개수
int cnt2 = 0;  // 이번 턴에 방문한 공기 개수
int cnt3 = 0;  // 시간

void dfs(vector<vector<int>>& grid, vector<vector<int>>& visited, int mx, int my, int sx, int sy) {
    visited[sy][sx] = 1;
    cnt2++;

    for (int i = 0; i < 4; i++) {
        int nx = sx + dx[i];
        int ny = sy + dy[i];

        if (nx < 0 || nx >= mx || ny < 0 || ny >= my) continue;
        if (visited[ny][nx]) continue;

        if (grid[ny][nx] == 1) {
            visited[ny][nx] = 1;
            grid[ny][nx] = 2;
            cnt++;
            continue;
        }

        if (grid[ny][nx] == 0) {
            dfs(grid, visited, mx, my, nx, ny);
        }
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int col = 0;
    int row = 0;
    cin >> col >> row;

    vector<vector<int>> grid(col, vector<int>(row));
    vector<vector<int>> visited(col, vector<int>(row));

    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            cin >> grid[i][j];
        }
    }

    int last = 0;

    while (1) {
        cnt = 0;
        cnt2 = 0;
        visited.assign(col, vector<int>(row, 0));

        dfs(grid, visited, row, col, 0, 0);

        if (cnt == 0) break;

        last = cnt;
        cnt3++;

        for (int i = 0; i < col; i++) {
            for (int j = 0; j < row; j++) {
                if (grid[i][j] == 2) {
                    grid[i][j] = 0;
                }
            }
        }
    }

    cout << cnt3 << '\n' << last;
}
*/
/*
int cnt = 0;
int EraseNode = 0;

void go(vector<vector<int>> Node, int NextNode) {
    if (Node[NextNode].size() == 0)cnt++;
    if (Node[NextNode].size() == 1 && Node[NextNode][0] == EraseNode)cnt++;
    for (int i = 0; i < Node[NextNode].size(); i++) {
        if (Node[NextNode][i] == EraseNode)continue;
        go(Node, Node[NextNode][i]);
    }
}
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);

    int NodeCnt = 0;
    int root = -1;
    cin >> NodeCnt;
    vector<vector<int>> Node(NodeCnt);

    for (int i = 0; i < NodeCnt; i++) {
        int temp;
        cin >> temp;

        if (temp == -1) {
            root = i;
            continue;
        }
        

        Node[temp].push_back(i);
    }

    cin >> EraseNode;
    if(EraseNode != root) go(Node, root);
    cout << cnt;
}
*/

/*
int N = 0;
int M = 0;
int totalLevel = 0;
vector<int> answer;

void go(const vector<vector<int>> &graph, int CurrentLevel, int NextNode) {
    CurrentLevel++;

    if (graph[NextNode].size() == 0) {

        if (totalLevel == CurrentLevel) {
            answer.push_back(NextNode);
        }

        if (totalLevel < CurrentLevel) {
            totalLevel = CurrentLevel;
            answer.clear();
            answer.push_back(NextNode);
        }
        
        
    }
    for (int i = 0; i < graph[NextNode].size(); i++) {
        
        go(graph, CurrentLevel, graph[NextNode][i]);
    }
}
int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);

    cin >> N >> M;
    vector<vector<int>> graph(N+1);
    for (int i = 0; i < M; i++) {
        int temp1;
        int temp2;

        cin >> temp1 >> temp2;
        graph[temp2].push_back(temp1);
    }

    for (int i = 1; i < N; i++) {
        go(graph, 0, i);
    }

    for (auto a : answer) {
        cout <<a<<' '<<'\n';
    }
}

*/
/*
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int N;
    cin >> N;

    vector<int> A(N);
    vector<int> answer(N, -1);
    stack<int> st;

    for (int i = 0; i < N; i++) {
        cin >> A[i];
    }

    for (int i = 0; i < N; i++) {
        while (!st.empty() && A[st.top()] < A[i]) {
            answer[st.top()] = A[i];
            st.pop();
        }
        st.push(i);
    }

    for (int i = 0; i < N; i++) {
        cout << answer[i] << " ";
    }
}
*/