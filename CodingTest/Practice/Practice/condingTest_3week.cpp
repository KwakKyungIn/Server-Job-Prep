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
#include <cstring>
using namespace std;

//a
/*
vector<pair<int, int>> house;
vector<pair<int, int>> chicken;
vector<int> selected;
int n, m = 0;
int answer = 1e9;
void combi(int idx, int cnt) {

	if (cnt == m) {

		int sum = 0;

		for (auto h : house) {

			int dist = 1e9;

			for (int c : selected) {
				auto ck = chicken[c];

				int d = abs(h.first - ck.first) + abs(h.second - ck.second);
				dist = min(dist, d);
			}

			sum += dist;
		}

		answer = min(answer, sum);
		return;
	}

	for (int i = idx; i < chicken.size(); i++) {
		selected.push_back(i);
		combi(i+1, cnt+1);
		selected.pop_back();
	}
}

int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	cin >> n >> m;
	vector<vector<int>> grid(n, vector<int>(n));
	vector<vector<int>> visited(n, vector<int>(n));
	


	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			int temp = 0;
			cin >> temp;
			grid[i][j] = temp;

			if (temp == 1) {
				house.push_back({ j, i });
			}

			if (temp == 2) {
				chicken.push_back({ j, i });
			}

		}
	}
	combi(0, 0);
	cout << answer;
}

*/

/*
int n, m;
const int dy[] = { -1,1,0,0 };
const int dx[] = { 0,0,-1,1 };
int ret = -1;

int bfs(int sy, int sx, const vector<vector<int>>& grid) {
	queue<pair<int, int>> q;
	vector<vector<int>> dist(n, vector<int>(m, -1));
	int ret = 0;

	q.push({ sy, sx });
	dist[sy][sx] = 0;

	while (!q.empty()) {
		auto cur = q.front();
		q.pop();

		int cy = cur.first;
		int cx = cur.second;

		ret = max(ret, dist[cy][cx]);

		for (int i = 0; i < 4; i++) {
			int ny = cy + dy[i];
			int nx = cx + dx[i];

			if (nx >= m || nx < 0 || ny >= n || ny < 0) continue;
			if (dist[ny][nx] != -1) continue;
			if (grid[ny][nx] == 1) continue;

			dist[ny][nx] = dist[cy][cx] + 1;
			q.push({ ny, nx });
		}
	}

	return ret;
}
int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	cin >> n >> m;
	vector<vector<int>> grid(n, vector<int>(m));
	int cnt = 0;

	for (int i = 0; i < n; i++) {
		string temp = "";
		cin >> temp;
		for (int j = 0; j < m; j++) {
			if (temp[j] == 'W') {
				grid[i][j] = 1;
			}
			if (temp[j] == 'L') {
				grid[i][j] = 0;
			}
		}
	}

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			if (grid[i][j] == 0) {
				cnt = max(cnt, bfs(i, j, grid));
			}
		}
	}

	cout << cnt;
}

*/
/*
int N, L, R;
int grid[51][51];
bool visited[51][51];
int dy[] = { -1, 1, 0, 0 };
int dx[] = { 0, 0, -1, 1 };

vector<pair<int, int>> unionCells; // 현재 탐색 중인 연합의 좌표들

// DFS: 연결된 모든 국가를 찾고 총 인구수를 반환
int dfs(int y, int x) {
	visited[y][x] = true;
	unionCells.push_back({ y, x });
	int sum = grid[y][x];

	for (int i = 0; i < 4; i++) {
		int ny = y + dy[i];
		int nx = x + dx[i];

		if (ny >= 0 && ny < N && nx >= 0 && nx < N && !visited[ny][nx]) {
			int diff = abs(grid[y][x] - grid[ny][nx]);
			if (diff >= L && diff <= R) {
				sum += dfs(ny, nx);
			}
		}
	}
	return sum;
}

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	cin >> N >> L >> R;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			cin >> grid[i][j];
		}
	}

	int days = 0;
	while (true) {
		bool isMoved = false;
		memset(visited, false, sizeof(visited));

		for (int i = 0; i < N; i++) {
			for (int j = 0; j < N; j++) {
				if (!visited[i][j]) {
					unionCells.clear();
					int totalPopulation = dfs(i, j);

					// 연합이 형성되었다면 (국가가 2개 이상)
					if (unionCells.size() > 1) {
						isMoved = true;
						int avgPopulation = totalPopulation / unionCells.size();
						for (auto pos : unionCells) {
							grid[pos.first][pos.second] = avgPopulation;
						}
					}
				}
			}
		}

		if (!isMoved) break; // 더 이상 이동이 없으면 탈출
		days++;
	}

	cout << days << "\n";
	return 0;
}
*/
/*

int R, C;
const int dy[] = { -1, 1, 0, 0 };
const int dx[] = { 0, 0, -1, 1 };

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	cin >> R >> C;

	vector<string> grid(R);
	for (int i = 0; i < R; i++) {
		cin >> grid[i];
	}

	queue<pair<int, int>> fireQ;
	queue<pair<int, int>> jihunQ;

	vector<vector<int>> fireTime(R, vector<int>(C, -1));
	vector<vector<int>> jihunTime(R, vector<int>(C, -1));

	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			if (grid[i][j] == 'F') {
				fireQ.push({ i, j });
				fireTime[i][j] = 0;
			}
			else if (grid[i][j] == 'J') {
				jihunQ.push({ i, j });
				jihunTime[i][j] = 0;
			}
		}
	}

	while (!fireQ.empty()) {
		int y = fireQ.front().first;
		int x = fireQ.front().second;
		fireQ.pop();

		for (int i = 0; i < 4; i++) {
			int ny = y + dy[i];
			int nx = x + dx[i];

			if (ny < 0 || ny >= R || nx < 0 || nx >= C) continue;
			if (grid[ny][nx] == '#') continue;
			if (fireTime[ny][nx] != -1) continue;

			fireTime[ny][nx] = fireTime[y][x] + 1;
			fireQ.push({ ny, nx });
		}
	}

	while (!jihunQ.empty()) {
		int y = jihunQ.front().first;
		int x = jihunQ.front().second;
		jihunQ.pop();

		for (int i = 0; i < 4; i++) {
			int ny = y + dy[i];
			int nx = x + dx[i];

			if (ny < 0 || ny >= R || nx < 0 || nx >= C) {
				cout << jihunTime[y][x] + 1;
				return 0;
			}

			if (grid[ny][nx] == '#') continue;
			if (jihunTime[ny][nx] != -1) continue;
			if (fireTime[ny][nx] != -1 && fireTime[ny][nx] <= jihunTime[y][x] + 1) continue;

			jihunTime[ny][nx] = jihunTime[y][x] + 1;
			jihunQ.push({ ny, nx });
		}
	}

	cout << "IMPOSSIBLE";
	return 0;
}
*/
/*
int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	int scvCount = 0;
	int cnt = 0;
	vector<int> health;
	cin >> scvCount;
	for (int i = 0; i < scvCount; i++) {
		int temp = 0;
		cin >> temp;
		health.push_back(temp);
	}

	sort(health.begin(), health.end(), greater<int>());

	do {
		
		cnt++;

		if (health.size() == 3) {
			health[0] = health[0] - 9;
			health[1] = health[1] - 3;
			health[2] = health[2] - 1;

			sort(health.begin(), health.end(), greater<int>());
		}
		else if (health.size() == 2) {
			health[0] = health[0] - 9;
			health[1] = health[1] - 3;

			sort(health.begin(), health.end(), greater<int>());

		}
		else if(health.size()==1){
			health[0] = health[0] - 9;

			sort(health.begin(), health.end(), greater<int>());

		}

		health.erase(remove_if(health.begin(), health.end(), [](int x) {
			return x <= 0;
			}), health.end());

	} while (health.size());

	cout << cnt;
}
*/
/*
int cnt;
bool findTn = 0;
int visited[100005] = { 0 };
int go(int sn, int tn) {
	queue<int> q;
	q.push(sn);
	int level = 0;
	while (!q.empty()) {
		int qSize = q.size();
		for (int i = 0; i < qSize; i++) {
			int temp = q.front();
			q.pop();
			if (temp < 0 || temp>100000)continue;
			if (visited[temp] == 1)continue;

			visited[temp] = 1;

			if (temp == tn) {
				cnt++;
				findTn = 1;
			}

			q.push(temp + 1);
			q.push(temp - 1);
			q.push(temp * 2);
			visited[temp] = 0;
		}

		
		if (findTn == 1) {
			break;
		}

		level++;
	}

	return level;
}
int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	int sn = 0;
	int tn = 0;
	int result = 0;
	cin >> sn >> tn;

	int level = go(sn, tn);

	cout << level << '\n' << cnt;

}
*/

/*

int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	int N=0, M = 0;
	pair<int, int> startPos;
	pair<int, int> endPos;
	cin >> N >> M;
	cin >> startPos.first >> startPos.second >> endPos.first >> endPos.second;
	startPos.first--;
	startPos.second--;
	endPos.first--;
	endPos.second--;
	vector<vector<char>> grid(N, vector<char>(M));
	vector<vector<int>> visited(N, vector<int>(M,-1));
	vector<vector<int>> visited2(N, vector<int>(M, -1));
	for (int i = 0; i < N; i++) {
		string temp;
		cin >> temp;
		for (int j = 0; j < M; j++) {
			grid[i][j] = temp[j];
		}
	}

	queue<pair<int,int>> q;
	queue<pair<int, int>> q2;
	q.push(startPos);
	visited[startPos.first][startPos.second] = 1;
	const int dy[] = { -1,1,0,0};
	const int dx[] = { 0,0,-1,1 };
	bool findPos = 0;
	int cnt = 0;
	while(!findPos) {
		cnt++;

		while (!q.empty()) {
			auto temp = q.front();
			q.pop();
			int cy = temp.first;
			int cx = temp.second;

			for (int i = 0; i < 4; i++) {
				int ny = cy + dy[i];
				int nx = cx + dx[i];

				if (ny < 0 || ny >= N || nx < 0 || nx >= M)continue;
				if (visited[ny][nx] == 1)continue;

				visited[ny][nx] = 1;

				if (grid[ny][nx] == '#') {
					q = queue<pair<int,int>>();
					findPos = 1;
					break;
				}
				
				if (grid[ny][nx] == '1') {
					grid[ny][nx] = '0';
					q2.push({ny,nx });
					continue;
				}

				if (grid[ny][nx] == '0') {
					q.push({ ny,nx });
				}
			}
		}

		visited = visited2;
		q = q2;
	}


	cout << cnt;
}
*/
/*
const int dy[] = { -1,1,0,0 };
const int dx[] = { 0,0,-1,1 };
int R, C;
vector<int> visited(26);

bool check(char c) {
	int k = c - 'A';
	if (visited[k] == 1) {
		return true;
	}
	else {
		return false;
	}
}

int go(vector<string> &grid, int sy,int sx) {
	visited[grid[sy][sx] - 'A'] = 1;
	int maxLen = 0;

	for (int i = 0; i < 4; i++) {
		int ny = dy[i] + sy;
		int nx = dx[i] + sx;

		if (ny >= R || ny < 0 || nx >= C || nx < 0)continue;
		if(check(grid[ny][nx]))continue;

		
		maxLen = max(maxLen, go(grid, ny, nx));

		

	}
	visited[grid[sy][sx] - 'A'] = 0;
	return maxLen+1;
}

int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	
	cin >> R >> C;

	vector<string> grid(R);

	for (int i = 0; i < R; i++) {
		cin >> grid[i];
	}

	cout<<go(grid, 0, 0);


}

*/

/*

vector<int> visited(10);
vector<char> checkingList;
int n;
bool check(int i) {

	if (visited[i] == 1) {
		return true;
	}
	else {
		return false;
	}
}

int go(int n) {
	visited[n] = 1;


	visited[n] = 0;
}
int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	cin >> n;

	for (int i = 0; i < n; i++) {
		char temp;
		cin >> temp;
		checkingList.push_back(temp);
	}
	for (int i = 0; i < 10; i++) {
		go(i);
	}
}

*/

/*
int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);

	int level = 0;
	cin >> level;
	int cnt = 0;
	cnt = (1 << level) - 1;

	vector<int> num(cnt);
	vector<vector<int>> answer(level);

	for (int i = 0; i < cnt; i++) {
		int temp;
		cin >> temp;
		num[i] = temp;
	}

	while (level!=1) {



		level--;
	}

}
*/

/*
const int dy[] = { -1,1,0,0 };
const int dx[] = { 0,0,-1,1 };
int R, C,K;
int countN;
void go(vector<string>& grid, vector<vector<int>> &visited, int sy, int sx, int dist) {
	visited[sy][sx] = 1;
	dist++;
	if (dist == K) {
		if (sy == 0 && sx==C-1) countN++;
		visited[sy][sx] = 0;
		return;
	}

	for (int i = 0; i < 4; i++) {
		int ny = dy[i] + sy;
		int nx = dx[i] + sx;

		if (ny >= R || ny < 0 || nx >= C || nx < 0)continue;
		if (visited[ny][nx]) continue;
		if (grid[ny][nx] == 'T')continue;

		go(grid, visited, ny, nx, dist);
	}
	visited[sy][sx] = 0;
}

int main() {
	ios_base::sync_with_stdio(false); cin.tie(NULL);


	cin >> R >> C>>K;

	vector<string> grid(R);
	vector<vector<int>> visited(R, vector<int>(C));

	for (int i = 0; i < R; i++) {
		cin >> grid[i];
	}

	go(grid, visited, R-1, 0, 0);

	cout<<countN;
}
*/
/*
int dy[] = { 0, -1, 1, 0, 0 };
int dx[] = { 0, 0, 0, -1, 1 };

bool canPlace(int y, int x, vector<vector<int>>& visited, vector<vector<int>>& cost) {
	for (int i = 0; i < 5; i++) {
		int ny = y + dy[i];
		int nx = x + dx[i];

		if (visited[ny][nx]) return false;
	}
	return true;
}

int getCost(int y, int x, vector<vector<int>>& cost) {
	int sum = 0;
	for (int i = 0; i < 5; i++) {
		int ny = y + dy[i];
		int nx = x + dx[i];
		sum += cost[ny][nx];
	}
	return sum;
}

void setVisited(int y, int x, vector<vector<int>>& visited, int val) {
	for (int i = 0; i < 5; i++) {
		int ny = y + dy[i];
		int nx = x + dx[i];
		visited[ny][nx] = val;
	}
}
int N;
int main() {

	cin >> N;
	vector<vector<int>> grid(N, vector<int>(N));
	vector<vector<int>> visited(N, vector<int>(N));

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			cin >> grid[i][j];
		}
	}


	for (int i = 0; i < N * N; i++) {
		for (int j = i + 1; j < N * N; j++) {
			for (int k = j + 1; k < N * N; k++) {

				int y1 = i / N, x1 = i % N;
				int y2 = j / N, x2 = j % N;
				int y3 = k / N, x3 = k % N;

				if (canPlace(y1, x1,visited, grid) &&
					canPlace(y2, x2,visited, grid) &&
					canPlace(y3, x3, visited, grid)) {

					int total = 0;

					setVisited(y1, x1, visited, 1);
					setVisited(y2, x2, visited, 1);
					setVisited(y3, x3, visited, 1);

					total += getCost(y1, x1,grid);
					total += getCost(y2, x2,grid);
					total += getCost(y3, x3,grid);

					// min 갱신

					setVisited(y1, x1, visited, 0);
					setVisited(y2, x2, visited, 0);
					setVisited(y3, x3, visited, 0);
				}


			}
		}
	}


}
*/

/*
int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	int N;

	cin >> N;
	int longcnt = N / 4;

	for (int i = 0; i < longcnt; i++) {
		cout << "long ";
	}
	cout << "int";
}

*/



int main() {
	int N, M;
	cin >> N >> M;
	vector<int> answer(N);
	for (int i = 0; i < N; i++) {
		answer[i] = i+1;
	}

	for (int i = 0; i < M; i++) {
		int temp1, temp2, temp3;
		cin >> temp1 >> temp2;
		temp3 = answer[temp1-1];
		answer[temp1-1] = answer[temp2-1];
		answer[temp2-1] = temp3;
	}
	for (auto a : answer) {
		cout << a << ' ';
	}
}