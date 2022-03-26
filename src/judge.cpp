#include <bits/stdc++.h>

#include "signal.h"
#include "sys/wait.h"
#include "unistd.h"
#define rep(i, n) for (int i = 0; i < n; i++)
#define PI pair<int, int>

using namespace std;
random_device seed_gen;
mt19937 engine(seed_gen());

struct UnionFind {
    int N;
    vector<int> par, siz;
    int find(int x) { return (par[x] == x ? x : par[x] = find(par[x])); }
    void merge(int x, int y) {
        x = find(x), y = find(y);
        if (x == y) return;
        if (siz[x] > siz[y]) swap(x, y);
        par[x] = y;
        siz[y] += siz[x];
    }
    UnionFind(int N) : N(N), siz(N, 1), par(N) {
        iota(par.begin(), par.end(), 0);
    }
};

pid_t __reactive_pid[2];

int __reactive_input[2], __reactive_output[2];
int reactive_start() {
    int pipe_c2p[2][2], pipe_p2c[2][2];

    signal(SIGPIPE, SIG_IGN);

    rep(i, 2) {
        if (pipe(pipe_c2p[i]) < 0 || pipe(pipe_p2c[i]) < 0) {
            fprintf(stderr, "pipe: failed to open pipes\n");
            return 1;
        }
        if ((__reactive_pid[i] = fork()) < 0) {
            fprintf(stderr, "fork: failed to fork\n");
            return 1;
        }
        if (__reactive_pid[i] == 0) {
            close(pipe_p2c[i][1]);
            close(pipe_c2p[i][0]);
            dup2(pipe_p2c[i][0], 0);
            dup2(pipe_c2p[i][1], 1);
            close(pipe_p2c[i][0]);
            close(pipe_c2p[i][1]);
            exit(system((!i ? "./player0" : "./player1")) ? 1 : 0);
        }
        close(pipe_p2c[i][0]);
        close(pipe_c2p[i][1]);
        __reactive_input[i] = pipe_p2c[i][1];
        __reactive_output[i] = pipe_c2p[i][0];
    }

    return 0;
}
void reactive_end() {
    int status;
    rep(i, 2) {
        close(__reactive_input[i]);
        waitpid(__reactive_pid[i], &status, WUNTRACED);
    }
}

void reactive_write(int fd, std::string buf) {
    write(fd, buf.c_str(), buf.size());
}

std::string reactive_read(int fd, int max_len = 100000) {
    static char buf[1024];
    static int len = 0;
    std::string result;
    while (result.size() < max_len) {
        if (!len) {
            len = read(fd, buf, std::min(1000, (int)(max_len - result.size())));
            if (!len) return result;
        }
        char *pos = (char *)memchr(buf, '\n', len);
        if (pos) {
            result += std::string(buf, pos - buf + 1);
            memmove(buf, pos + 1, len - (pos + 1 - buf));
            len -= pos - buf + 1;
            return result;
        } else {
            result += std::string(buf, len);
            len = 0;
        }
    }
    return result;
}

int dx[] = {-1, 0, 1, 0}, dy[] = {0, 1, 0, -1};

struct Act {
    int type, x, y;
};
struct Game {
    vector<vector<bool>> C;
    vector<vector<bool>> wall_hrz, wall_vert;
    vector<int> X, Y, wall_used, score;
    bool is_finished = false;

    int MAX_WALL = 10;
    int turn = 0;
    vector<Act> acts;

    bool isConnected() {
        UnionFind uf(81);
        rep(i, 9) rep(j, 9) {
            if (i < 8 && !wall_hrz[i][j]) uf.merge(i * 9 + j, (i + 1) * 9 + j);
            if (j < 8 && !wall_vert[i][j]) uf.merge(i * 9 + j, i * 9 + j + 1);
        }
        return uf.siz[uf.find(0)] == 81;
    }

    bool isOK(Act act) {
        if (act.type == 0) {
            if (act.x == 0)
                return X[turn] && !wall_hrz[X[turn] - 1][Y[turn]];
            else if (act.x == 1)
                return Y[turn] != 8 && !wall_vert[X[turn]][Y[turn]];
            else if (act.x == 2)
                return X[turn] != 8 && !wall_hrz[X[turn]][Y[turn]];
            else if (act.x == 3)
                return Y[turn] && !wall_vert[X[turn]][Y[turn] - 1];
            else
                return false;
        } else if (act.type == 1) {
            if (act.x < 0 || act.x > 7 || act.y < 0 || act.y > 7) return false;
            if (wall_used[turn] == MAX_WALL) return false;
            if (wall_hrz[act.x][act.y] || wall_hrz[act.x][act.y + 1])
                return false;
            wall_hrz[act.x][act.y] = wall_hrz[act.x][act.y + 1] = true;
            bool res = isConnected();
            wall_hrz[act.x][act.y] = wall_hrz[act.x][act.y + 1] = false;
            return res;
        } else if (act.type == 2) {
            if (act.x < 0 || act.x > 7 || act.y < 0 || act.y > 7) return false;
            if (wall_used[turn] == MAX_WALL) return false;
            if (wall_vert[act.x][act.y] || wall_vert[act.x + 1][act.y])
                return false;
            wall_vert[act.x][act.y] = wall_vert[act.x + 1][act.y] = true;
            bool res = isConnected();
            wall_vert[act.x][act.y] = wall_vert[act.x + 1][act.y] = false;
            return res;
        } else
            return false;
    }

    vector<Act> candidateActs() {
        vector<Act> res;
        rep(i, 4) {
            if (isOK(Act{0, i, 0})) res.push_back(Act{0, i, 0});
        }
        rep(i, 9) rep(j, 9) {
            if (isOK(Act{1, i, j})) res.push_back(Act{1, i, j});
            if (isOK(Act{2, i, j})) res.push_back(Act{2, i, j});
        }
        return res;
    }

    bool applyAct(Act act, bool check = true) {
        if (check && !isOK(act)) return false;
        if (act.type == 0) {
            X[turn] += dx[act.x];
            Y[turn] += dy[act.x];
            if (C[X[turn]][Y[turn]]) {
                score[turn]++;
                C[X[turn]][Y[turn]] = false;
            }
        } else if (act.type == 1) {
            wall_hrz[act.x][act.y] = wall_hrz[act.x][act.y + 1] = true;
            wall_used[turn]++;
        } else {
            wall_vert[act.x][act.y] = wall_vert[act.x + 1][act.y] = true;
            wall_used[turn]++;
        }
        bool flag = false;
        rep(i, 9) rep(j, 9) {
            if (C[i][j]) flag = true;
        }
        if (!flag) is_finished = true;
        acts.push_back(act);
        turn ^= 1;
        return true;
    }

    void printBoard() {
        cout << "Score: " << score[0] << " " << score[1] << "\n"
             << "Wall counter: " << wall_used[0] << " " << wall_used[1] << endl
             << endl;
        rep(i, 9) {
            rep(j, 9) {
                if (i == X[0] && j == Y[0] && i == X[1] && j == Y[1])
                    cout << " AB";
                else if (i == X[0] && j == Y[0])
                    cout << " A ";
                else if (i == X[1] && j == Y[1])
                    cout << " B ";
                else {
                    if (C[i][j])
                        cout << " # ";
                    else
                        cout << " . ";
                }
                if (j < 8) cout << (wall_vert[i][j] ? " | " : "   ");
            }
            cout << endl;
            if (i != 8) {
                rep(j, 9) {
                    if (j) cout << "   ";
                    cout << (wall_hrz[i][j] ? "---" : "   ");
                }
            }
            cout << endl;
        }
        cout << endl;
    }

    Game()
        : C(9, vector<bool>(9)),
          wall_hrz(9, vector<bool>(9)),
          wall_vert(9, vector<bool>(9)),
          X(2),
          Y(2),
          wall_used(2),
          score(2) {
        wall_used[0] = wall_used[1] = 0;
        score[0] = score[1] = 0;
        X[0] = 8, X[1] = 0, Y[0] = Y[1] = 4;
    }
};

Game generate_case() {
    Game game;
    rep(i, 9) rep(j, 9) {
        if (4 < i)
            game.C[i][j] = game.C[8 - i][j];
        else if (!(i == 0 && j == 4) && !(i == 8 && j == 4) &&
                 engine() % 5 < 3) {
            game.C[i][j] = true;
        } else {
            game.C[i][j] = false;
        }
    }
    return game;
}

ofstream ofs("judge_output.txt");
string interaction() {
    reactive_write(__reactive_input[0], "First\n");
    reactive_write(__reactive_input[1], "Second\n");

    Game game = generate_case();

    rep(i, 9) {
        string line;
        rep(j, 9) {
            if (game.C[i][j])
                line += '#';
            else
                line += '.';
        }
        ofs << line << endl;
        line += "\n";
        rep(k, 2) reactive_write(__reactive_input[k], line);
    }

    double tim[2] = {0, 0};
    chrono::system_clock::time_point clk = chrono::system_clock::now();
    rep(_, 150) {
        string line = reactive_read(__reactive_output[game.turn]);
        stringstream ss;
        ss << line;

        Act act;
        ss >> act.type;
        if (act.type == 0) {
            ss >> act.x;
        } else {
            ss >> act.x >> act.y;
        }

        tim[game.turn] +=
            static_cast<double>(chrono::duration_cast<chrono::microseconds>(
                                    chrono::system_clock::now() - clk)
                                    .count() /
                                1000000.0);
        if (tim[game.turn] > 75) {
            if(!game.turn)return "TLE -";
            else return "- TLE";
        }

        if (!game.applyAct(act)) {
            if(!game.turn)return "WA -";
            else return "- WA";
        }
        //game.printBoard();
        if (act.type == 0) {
            ofs << _ << " " << 0 << " " << act.x << endl;
            reactive_write(__reactive_input[game.turn],
                           "0 " + to_string(act.x) + "\n");
        } else {
            ofs << _ << " " << act.type << " " << act.x << " " << act.y << endl;
            reactive_write(__reactive_input[game.turn],
                           to_string(act.type) + " " + to_string(act.x) + " " +
                               to_string(act.y) + "\n");
        }
        if (game.is_finished) break;
        clk = chrono::system_clock::now();
    }
    return to_string(game.score[0])+" "+to_string(game.score[1]);
}
int main() {
    reactive_start();
    ofs << interaction() << endl;
    ofs << "END" << endl;
    rep(i, 2) reactive_write(__reactive_input[i], "3\n");
    reactive_end();
}