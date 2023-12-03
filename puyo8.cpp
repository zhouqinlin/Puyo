#include <curses.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <vector>
#include <set>
#include <fstream>
#include <algorithm>
#include <string>

class PuyoArray;
class PuyoArrayActive;
class PuyoArrayStack;
class PuyoControl;
class PuyoGame;

// ぷよの色を表すの列挙型
// NONEが無し，RED,BLUE,..が色を表す
enum puyocolor
{
	NONE,
	RED,
	BLUE,
	GREEN,
	YELLOW,
	PURPLE
};

class PuyoArray
{
public:
	PuyoArray() : data(NULL), data_line(0), data_column(0) {}

	~PuyoArray()
	{
		Release();
	}

	void ChangeSize(unsigned int line, unsigned int column)
	{
		Release();
		data = new puyocolor[line * column];
		data_line = line;
		data_column = column;
	}

	unsigned int GetLine()
	{
		return data_line;
	}

	unsigned int GetColumn()
	{
		return data_column;
	}

	puyocolor GetValue(unsigned int y, unsigned int x)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			// 引数の値が正しくない
			return NONE;
		}
		return data[y * GetColumn() + x];
	}

	void SetValue(unsigned int y, unsigned int x, puyocolor puyodata)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			// 引数の値が正しくない
			return;
		}
		data[y * GetColumn() + x] = puyodata;
	}

	int CountPuyo()
	{
		int count = 0;
		for (int y = 0; y < GetLine(); y++)
		{
			for (int x = 0; x < GetColumn(); x++)
			{
				if (GetValue(y, x) != NONE)
				{
					count++;
				}
			}
		}
		return count;
	}

private:
	puyocolor *data;
	unsigned int data_line;
	unsigned int data_column;

	void Release()
	{
		if (data == NULL)
		{
			return;
		}
		delete[] data;
		data = NULL;
	}
};

class PuyoArrayActive : public PuyoArray
{
private:
	int puyorotate;
	puyocolor *nextpuyo;

	void ReleaseNextPuyo()
	{
		if (nextpuyo == NULL)
		{
			return;
		}
		delete[] nextpuyo;
		nextpuyo = NULL;
	}

public:
	PuyoArrayActive()
	{
		puyorotate = 0;
		nextpuyo = new puyocolor[3 * 2];
	}

	~PuyoArrayActive()
	{
		ReleaseNextPuyo();
	}

	int GetPuyoRotate() const
	{
		return puyorotate;
	}
	void SetPuyoRate(int rotate)
	{
		puyorotate = rotate;
	}

	puyocolor GetNextPuyoValue(unsigned int y, unsigned int x)
	{
		if (y >= 3 || x >= 3)
		{
			// 引数の値が正しくない
			return NONE;
		}
		return nextpuyo[y * 2 + x];
	}

	void SetNextPuyoValue(unsigned int y, unsigned int x, puyocolor puyodata)
	{
		if (y >= 3 || x >= 3)
		{
			// 引数の値が正しくない
			return;
		}
		nextpuyo[y * 2 + x] = puyodata;
	}
};

class PuyoArrayStack : public PuyoArray
{
private:
	int score;
	int nowscore;

public:
	PuyoArrayStack()
	{
		score = 0;
		nowscore = 0;
	}
	int GetScore() const
	{
		return score;
	}
	void AddScore(int num)
	{
		score += num;
	}
	void SetScore(int num)
	{
		score = num;
	}

	int GetNowscore() const
	{
		return nowscore;
	}
	void SetNowScore(int num)
	{
		nowscore = num;
	}
};

class PuyoControl
{
public:
	void GeneratePuyo(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		if (stack.GetValue(0, 5) != NONE || stack.GetValue(0, 6) != NONE)
		{
			return;
		}

		active.SetPuyoRate(0);
		SetChainCount(0);

		GenerateNextPuyo(active, stack);

		active.SetValue(0, 5, active.GetNextPuyoValue(0, 0));
		active.SetValue(0, 6, active.GetNextPuyoValue(0, 1));
		// active.SetValue(0, 5, RED);
		// active.SetValue(0, 6, BLUE);
	}

private:
	void GenerateNextPuyo(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		if (active.GetNextPuyoValue(1, 0) == NONE || active.GetNextPuyoValue(1, 1) == NONE)
		{
			active.SetNextPuyoValue(0, 0, RandomColor());
			active.SetNextPuyoValue(0, 1, RandomColor());
			active.SetNextPuyoValue(1, 0, RandomColor());
			active.SetNextPuyoValue(1, 1, RandomColor());
		}
		else
		{
			active.SetNextPuyoValue(0, 0, active.GetNextPuyoValue(1, 0));
			active.SetNextPuyoValue(0, 1, active.GetNextPuyoValue(1, 1));
			active.SetNextPuyoValue(1, 0, active.GetNextPuyoValue(2, 0));
			active.SetNextPuyoValue(1, 1, active.GetNextPuyoValue(2, 1));
		}

		active.SetNextPuyoValue(2, 0, RandomColor());
		active.SetNextPuyoValue(2, 1, RandomColor());
	}

public:
	// 着地判定
	bool LandingPuyo(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		bool landed = false;
		int ly, lx = 0;
		for (int y = stack.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) != NONE && (y == active.GetLine() - 1 || stack.GetValue(y + 1, x) != NONE))
				{
					stack.SetValue(y, x, active.GetValue(y, x));
					active.SetValue(y, x, NONE);
					ly = y;
					lx = x;
				}
			}
		}

		if (active.CountPuyo() == 1)
		{
			for (int x = lx - 1; x <= lx + 1; x++)
			{
				if (x < 0 || x > active.GetColumn() - 1)
				{
					continue;
				}
				if (active.GetValue(ly, x) != NONE)
				{
					stack.SetValue(ly, x, active.GetValue(ly, x));
					active.SetValue(ly, x, NONE);
				}
			}
			LandFloating(active, stack);
		}

		if (active.CountPuyo() == 0)
		{
			landed = true;
			if (GetChainCount() == 0)
			{
				ClearScoreDisplay();
			}
		}
		return landed;
	}

	// 浮いた着地済みぷよを落下中ぷよに変換
	bool StackFloating(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		bool floating = false;
		for (int y = stack.GetLine() - 2; y >= 0; y--)
		{
			for (int x = 0; x < stack.GetColumn(); x++)
			{
				if (stack.GetValue(y, x) != NONE && stack.GetValue(y + 1, x) == NONE)
				{
					active.SetValue(y, x, stack.GetValue(y, x));
					stack.SetValue(y, x, NONE);
					floating = true;
				}
			}
		}
		return floating;
	}

	// 浮いた着地済みぷよの着地処理
	bool LandFloating(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		if (StackFloating(active, stack))
		{
			while (LandingPuyo(active, stack) != true)
			{
				MoveDown(active, stack);
				_Display(active, stack);
				usleep(150000);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	// 左移動
	void MoveLeft(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		for (int y = 0; y < active.GetLine() - 1; y++)
		{
			for (int x = 1; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) == NONE)
				{
					continue;
				}
				// 垂直に並んだ2つのぷよの下の1つ移動できなければ2つとも移動できない
				if (active.GetValue(y + 1, x) != NONE && stack.GetValue(y + 1, x - 1) != NONE)
				{
					return;
				}
				// 移動先の位置では落下中のぷよや着地済みぷよがなければ移動
				if (active.GetValue(y, x - 1) == NONE && stack.GetValue(y, x - 1) == NONE)
				{
					active.SetValue(y, x - 1, active.GetValue(y, x));
					active.SetValue(y, x, NONE);
				}
			}
		}
	}

	// 右移動
	void MoveRight(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		for (int y = 0; y < active.GetLine() - 1; y++)
		{
			for (int x = active.GetColumn() - 2; x >= 0; x--)
			{
				if (active.GetValue(y, x) == NONE)
				{
					continue;
				}
				// 垂直に並んだ2つのぷよの下の1つ移動できなければ2つとも移動できない
				if (active.GetValue(y + 1, x) != NONE && stack.GetValue(y + 1, x + 1) != NONE)
				{
					return;
				}
				// 移動先の位置では落下中のぷよや着地済みぷよがなければ移動
				if (active.GetValue(y, x + 1) == NONE && stack.GetValue(y, x + 1) == NONE)
				{
					active.SetValue(y, x + 1, active.GetValue(y, x));
					active.SetValue(y, x, NONE);
				}
			}
		}
	}

	// 下移動
	void MoveDown(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		for (int y = active.GetLine() - 2; y >= 0; y--)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) == NONE)
				{
					continue;
				}
				if (active.GetValue(y + 1, x) == NONE && stack.GetValue(y + 1, x) == NONE)
				{
					active.SetValue(y + 1, x, active.GetValue(y, x));
					active.SetValue(y, x, NONE);
				}
			}
		}
	}

	// ぷよ消滅処理を全座標で行う
	// 消滅したぷよの数を返す
	// 得点計算を行う
	int VanishPuyo(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		int vanishednumber = 0;
		int vanishnum = 0;
		int colorCount = 0;
		int connectionBonusValue = 0;
		int colorBonusValue = 0;
		int chainBonusValue = 0;
		int totalBonus = 0;
		int score = 0;
		puyocolor color;
		std::vector<puyocolor> vanishedColors;

		int chainBonus[] = {0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512};
		int connectionBonus[] = {0, 2, 3, 4, 5, 6, 7, 10};
		int colorBonus[] = {0, 3, 6, 12, 24};

		for (int y = 0; y < stack.GetLine(); y++)
		{
			for (int x = 0; x < stack.GetColumn(); x++)
			{
				color = stack.GetValue(y, x);
				vanishnum = VanishPuyo(active, stack, y, x);

				if (vanishnum > 0)
				{
					// 連結ボーナス計算
					if (vanishnum > 11)
					{
						connectionBonusValue += connectionBonus[sizeof(connectionBonus) / sizeof(connectionBonus[0]) - 1];
					}
					else
					{
						connectionBonusValue += connectionBonus[vanishnum - 4];
					}
					vanishednumber += vanishnum;
					vanishedColors.push_back(color);
				}
			}
		}

		if (vanishednumber == 0)
		{
			return 0;
		}
		// 色数ボーナスの計算
		std::set<puyocolor> uniqueColors(vanishedColors.begin(), vanishedColors.end());
		colorCount = uniqueColors.size();
		colorBonusValue = colorBonus[colorCount - 1];
		// 連鎖ボーナスの計算
		chainBonusValue = chainBonus[GetChainCount()];
		AddChainCount(1);
		// 得点計算
		totalBonus = chainBonusValue + connectionBonusValue + colorBonusValue;
		if (totalBonus == 0)
		{
			totalBonus = 1;
		}
		score = vanishednumber * totalBonus * 10;
		stack.AddScore(score);
		stack.SetNowScore(score);
		_ScoreDisplay(active, stack);

		return vanishednumber;
	}

	// ぷよ消滅処理を座標(x,y)で行う
	// 消滅したぷよの数を返す
	int VanishPuyo(PuyoArrayActive &active, PuyoArrayStack &stack, unsigned int y, unsigned int x)
	{
		// 判定個所にぷよがなければ処理終了
		if (stack.GetValue(y, x) == NONE)
		{
			return 0;
		}

		// 判定状態を表す列挙型
		// NOCHECK判定未実施，CHECKINGが判定対象，CHECKEDが判定済み
		enum checkstate
		{
			NOCHECK,
			CHECKING,
			CHECKED
		};

		// 判定結果格納用の配列
		enum checkstate *field_array_check;
		field_array_check = new enum checkstate[stack.GetLine() * stack.GetColumn()];

		// 配列初期化
		for (int i = 0; i < stack.GetLine() * stack.GetColumn(); i++)
		{
			field_array_check[i] = NOCHECK;
		}

		// 座標(x,y)を判定対象にする
		field_array_check[y * stack.GetColumn() + x] = CHECKING;
		puyocolor color = stack.GetValue(y, x);
		// 判定対象が1つもなくなるまで，判定対象の上下左右に同じ色のぷよがあるか確認し，あれば新たな判定対象にする
		bool checkagain = true;
		while (checkagain)
		{
			checkagain = false;

			for (int yy = 0; yy < stack.GetLine(); yy++)
			{
				for (int xx = 0; xx < stack.GetColumn(); xx++)
				{
					//(xx,yy)に判定対象がある場合
					if (field_array_check[yy * stack.GetColumn() + xx] == CHECKING)
					{
						//(xx+1,yy)の判定
						if (xx < stack.GetColumn() - 1)
						{
							//(xx+1,yy)と(xx,yy)のぷよの色が同じで，(xx+1,yy)のぷよが判定未実施か確認
							if (stack.GetValue(yy, xx + 1) == stack.GetValue(yy, xx) && field_array_check[yy * stack.GetColumn() + (xx + 1)] == NOCHECK)
							{
								//(xx+1,yy)を判定対象にする
								field_array_check[yy * stack.GetColumn() + (xx + 1)] = CHECKING;
								checkagain = true;
							}
						}

						//(xx-1,yy)の判定
						if (xx > 0)
						{
							if (stack.GetValue(yy, xx - 1) == stack.GetValue(yy, xx) && field_array_check[yy * stack.GetColumn() + (xx - 1)] == NOCHECK)
							{
								field_array_check[yy * stack.GetColumn() + (xx - 1)] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy+1)の判定
						if (yy < stack.GetLine() - 1)
						{
							if (stack.GetValue(yy + 1, xx) == stack.GetValue(yy, xx) && field_array_check[(yy + 1) * stack.GetColumn() + xx] == NOCHECK)
							{
								field_array_check[(yy + 1) * stack.GetColumn() + xx] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy-1)の判定
						if (yy > 0)
						{
							if (stack.GetValue(yy - 1, xx) == stack.GetValue(yy, xx) && field_array_check[(yy - 1) * stack.GetColumn() + xx] == NOCHECK)
							{
								field_array_check[(yy - 1) * stack.GetColumn() + xx] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy)を判定済みにする
						field_array_check[yy * stack.GetColumn() + xx] = CHECKED;
					}
				}
			}
		}

		// 判定済みの数をカウント
		int puyocount = 0;
		for (int i = 0; i < stack.GetLine() * stack.GetColumn(); i++)
		{
			if (field_array_check[i] == CHECKED)
			{
				puyocount++;
			}
		}

		// 4個以上あれば，判定済み座標のぷよを消す
		int vanishednumber = 0;
		if (4 <= puyocount)
		{
			for (int i = 0; i <= 2; i++)
			{
				vanishednumber = 0;
				// i の奇偶によってパターンを切り替える
				bool isVanished = (i % 2 == 0);
				// puyostack内の対応する位置の値を更新
				for (int yy = 0; yy < stack.GetLine(); yy++)
				{
					for (int xx = 0; xx < stack.GetColumn(); xx++)
					{
						if (field_array_check[yy * stack.GetColumn() + xx] == CHECKED)
						{
							if (isVanished)
							{
								stack.SetValue(yy, xx, NONE);
								vanishednumber++;
							}
							else
							{
								stack.SetValue(yy, xx, color);
							}
						}
					}
				}
				_Display(active, stack);
				usleep(300000);
			}
		}

		// メモリ解放
		delete[] field_array_check;

		return vanishednumber;
	}

	void Rotate(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		for (int y = 0; y < active.GetLine() - 1; y++)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) != NONE)
				{
					switch (active.GetPuyoRotate())
					{
					case 0:
						if (y == active.GetLine() - 1 || (stack.GetValue(y + 1, x) != NONE || stack.GetValue(y + 1, x + 1) != NONE))
						{
							return;
						}
						active.SetValue(y + 1, x, active.GetValue(y, x + 1));
						active.SetValue(y, x + 1, NONE);
						active.SetPuyoRate(1);
						return;
					case 1:
						if (x == 0 || (stack.GetValue(y, x - 1) != NONE || stack.GetValue(y + 1, x - 1) != NONE))
						{
							return;
						}
						active.SetValue(y, x - 1, active.GetValue(y + 1, x));
						active.SetValue(y + 1, x, NONE);
						active.SetPuyoRate(2);
						return;
					case 2:
						if (y == 0 || (stack.GetValue(y - 1, x + 1) != NONE || stack.GetValue(y - 1, x) != NONE))
						{
							return;
						}
						active.SetValue(y - 1, x + 1, active.GetValue(y, x));
						active.SetValue(y, x, NONE);
						active.SetPuyoRate(3);
						return;
					case 3:
						if (x == active.GetColumn() - 1 || (stack.GetValue(y + 1, x + 1) != NONE || stack.GetValue(y, x + 1) != NONE))
						{
							return;
						}
						active.SetValue(y + 1, x + 1, active.GetValue(y, x));
						active.SetValue(y, x, NONE);
						active.SetPuyoRate(0);
						return;
					default:
						active.SetPuyoRate(0);
						return;
					}
				}
			}
		}
		return;
	}

	void ResetGame(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		for (int y = 0; y < active.GetLine(); y++)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) != NONE)
				{

					active.SetValue(y, x, NONE);
				}
				if (stack.GetValue(y, x) != NONE)
				{

					stack.SetValue(y, x, NONE);
				}
			}
		}
		stack.SetNowScore(0);
		stack.SetScore(0);
	}

	// 落下中ぷよは操作可能か判定
	bool CanMove(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		if (active.GetValue(0, 5) != NONE && active.GetValue(0, 6) != NONE)
		{
			return false;
		}

		if (active.CountPuyo() == 2)
		{
			return true;
		}

		return false;
	}

private:
	void _Display(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		// 文字の色と背景の色のペアを初期化する
		init_pair(0, COLOR_WHITE, COLOR_BLACK);
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_BLUE, COLOR_BLACK);
		init_pair(3, COLOR_GREEN, COLOR_BLACK);
		init_pair(4, COLOR_YELLOW, COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

		// ぷよ表示
		for (int y = 0; y < active.GetLine(); y++)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) != NONE)
				{
					switch (active.GetValue(y, x))
					{
					case RED:
						attrset(COLOR_PAIR(1));
						mvaddch(y, x, 'R');
						break;
					case BLUE:
						attrset(COLOR_PAIR(2));
						mvaddch(y, x, 'B');
						break;
					case GREEN:
						attrset(COLOR_PAIR(3));
						mvaddch(y, x, 'G');
						break;
					case YELLOW:
						attrset(COLOR_PAIR(4));
						mvaddch(y, x, 'Y');
						break;
					case PURPLE:
						attrset(COLOR_PAIR(5));
						mvaddch(y, x, 'P');
						break;
					default:
						break;
					}
				}
				else
				{
					switch (stack.GetValue(y, x))
					{
					case NONE:
						attrset(COLOR_PAIR(0));
						mvaddch(y, x, '.');
						break;
					case RED:
						attrset(COLOR_PAIR(1));
						mvaddch(y, x, 'R');
						break;
					case BLUE:
						attrset(COLOR_PAIR(2));
						mvaddch(y, x, 'B');
						break;
					case GREEN:
						attrset(COLOR_PAIR(3));
						mvaddch(y, x, 'G');
						break;
					case YELLOW:
						attrset(COLOR_PAIR(4));
						mvaddch(y, x, 'Y');
						break;
					case PURPLE:
						attrset(COLOR_PAIR(5));
						mvaddch(y, x, 'P');
						break;
					default:
						mvaddch(y, x, '?');
						break;
					}
				}
			}
		}

		refresh();
	}

	void _ScoreDisplay(PuyoArrayActive &active, PuyoArrayStack &stack)
	{
		int addScore = stack.GetNowscore();
		int chain = GetChainCount();

		if (addScore > 0)
		{
			mvprintw(4, COLS - 29, "+ %d     ", addScore);
		}

		if (chain > 1)
		{
			mvprintw(4, COLS - 14, "Chain %d!", chain);
		}

		if (stack.CountPuyo() == 0)
		{
			mvprintw(LINES / 2 + 1, COLS / 2 - 10, "ALL CLEAR!");
		}

		refresh();
	}

	void ClearScoreDisplay()
	{
		mvprintw(4, COLS - 29, "                            ");
		mvprintw(LINES / 2 + 1, COLS / 2 - 10, "           ");
		refresh();
	}

	// ランダムなぷよ色を生成
	puyocolor RandomColor()
	{
		int colornumber = GetColorNum();

		int randomIndex = 1 + std::rand() % colornumber;

		// ランダムな整数を列挙型の値に変換する
		puyocolor newpuyo;
		newpuyo = static_cast<puyocolor>(randomIndex);
		return newpuyo;

		/*
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distribution(0, colornumber - 1);

		int randomIndex = distribution(gen) + 1;
		puyocolor newpuyo;
		newpuyo = static_cast<puyocolor>(randomIndex);
		return newpuyo;
		*/
	}

private:
	int ChainCount;
	int MaxChain;
	int ColorNum;

public:
	PuyoControl()
	{
		// 連鎖数
		ChainCount = 0;

		// 最大連鎖数
		MaxChain = 0;

		// ぷよの色数
		ColorNum = 4;

		// 乱数生成器を初期化する
		std::srand(std::time(NULL));
	}

	int GetChainCount() const
	{
		return ChainCount;
	}
	void AddChainCount(int num)
	{
		ChainCount += num;
		if (ChainCount > MaxChain)
		{
			SetMaxChain(ChainCount);
		}
	}
	void SetChainCount(int num)
	{
		ChainCount = num;
	}

	int GetMaxChain() const
	{
		return MaxChain;
	}
	void SetMaxChain(int num)
	{
		MaxChain = num;
	}

	int GetColorNum() const
	{
		return ColorNum;
	}
	void SetColorNum(int num)
	{
		ColorNum = num;
	}
};

class PuyoGame
{
public:
	PuyoGame()
	{
		waitCount = 20000;
		maxGameDuration = 600;
	}

	~PuyoGame()
	{
		endwin();
	}

	void Run()
	{
		// 画面の初期化
		initscr();
		// カラー属性を扱うための初期化
		start_color();
		// キーを押しても画面に表示しない
		noecho();
		// キー入力を即座に受け付ける
		cbreak();
		curs_set(0);
		// キー入力受付方法指定
		keypad(stdscr, TRUE);
		// キー入力非ブロッキングモード
		timeout(0);

		// Read Scoreboard from file
		playerInfoList = LoadPlayerInfo("scoreboard.txt");

		bool end = false;
		while (!end)
		{
			int choice = ShowMainMenu();
			switch (choice)
			{
			case 1:
				// Start game
				RunGame();
				break;
			case 2:
				// check Scoreboard
				ShowScoreboard();
				break;
			case 3:
				// Settings
				ShowSettingMenu();
				break;
			case 4:
				// exit game
				end = true;
				break;
			default:
				break;
			}
		}
		// exit game
		endwin();
	}

private:
	struct PlayerInfo
	{
		std::string name;
		int score;

		// Define a comparison function for sorting by score in descending order
		bool operator<(const PlayerInfo &other) const
		{
			return score > other.score;
		}
	};

	PuyoArrayActive active;
	PuyoArrayStack stack;
	PuyoControl control;
	std::vector<PlayerInfo> playerInfoList;
	std::time_t gameStartTime;
	int waitCount;
	int maxGameDuration;

	std::vector<PlayerInfo> LoadPlayerInfo(const std::string &filename)
	{
		std::vector<PlayerInfo> playerInfoList;
		std::ifstream file(filename.c_str());

		if (!file.is_open())
		{
			// File does not exist, create empty file
			std::ofstream createFile(filename.c_str());
			createFile.close();
		}
		else
		{
			PlayerInfo playerInfo;
			while (file >> playerInfo.name >> playerInfo.score)
			{
				playerInfoList.push_back(playerInfo);
			}
			file.close();
		}
		// Sort the player info list by score in descending order
		std::sort(playerInfoList.begin(), playerInfoList.end());

		return playerInfoList;
	}

	// Save player information to file
	void SavePlayerInfo(const std::string &filename)
	{
		std::ofstream file(filename.c_str());

		// Sort the player info list by score in descending order
		std::sort(playerInfoList.begin(), playerInfoList.end());

		if (file.is_open())
		{
			for (std::vector<PlayerInfo>::const_iterator it = playerInfoList.begin(); it != playerInfoList.end(); ++it)
			{
				const PlayerInfo &playerInfo = *it;
				file << playerInfo.name.c_str() << " " << playerInfo.score << std::endl;
			}
			file.close();
		}
	}

	int ShowMainMenu()
	{
		clear();

		int choice = 0;
		int highlight = 0;
		int ch;

		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_BLUE, COLOR_BLACK);
		init_pair(3, COLOR_YELLOW, COLOR_BLACK);
		init_pair(4, COLOR_GREEN, COLOR_BLACK);

		attron(COLOR_PAIR(1));
		mvprintw(LINES / 2 - 3, COLS / 2 - 4, "P");
		attroff(COLOR_PAIR(1));

		attron(COLOR_PAIR(3));
		mvprintw(LINES / 2 - 3, COLS / 2 - 3, "u");
		attroff(COLOR_PAIR(3));

		attron(COLOR_PAIR(4));
		mvprintw(LINES / 2 - 3, COLS / 2 - 2, "y");
		attroff(COLOR_PAIR(4));

		attron(COLOR_PAIR(2));
		mvprintw(LINES / 2 - 3, COLS / 2 - 1, "o");
		attroff(COLOR_PAIR(2));

		attron(COLOR_PAIR(1));
		mvprintw(LINES / 2 - 3, COLS / 2, " P");
		attroff(COLOR_PAIR(1));

		attron(COLOR_PAIR(3));
		mvprintw(LINES / 2 - 3, COLS / 2 + 2, "u");
		attroff(COLOR_PAIR(3));

		attron(COLOR_PAIR(4));
		mvprintw(LINES / 2 - 3, COLS / 2 + 3, "y");
		attroff(COLOR_PAIR(4));

		attron(COLOR_PAIR(2));
		mvprintw(LINES / 2 - 3, COLS / 2 + 4, "o");
		attroff(COLOR_PAIR(2));

		mvprintw(LINES - 1, 0, "Press Up Down Enter or Number Key to Choose");

		while (1)
		{

			// Display main menu options
			mvprintw(LINES / 2 - 1, COLS / 2 - 6, "1. Start     ");
			mvprintw(LINES / 2, COLS / 2 - 6, "2. Scoreboard");
			mvprintw(LINES / 2 + 1, COLS / 2 - 6, "3. Settings  ");
			mvprintw(LINES / 2 + 2, COLS / 2 - 6, "4. Quit      ");

			// Highlight the current option
			mvchgat(LINES / 2 + highlight - 1, COLS / 2 - 6, 13, A_REVERSE, 0, NULL);

			ch = getch();

			switch (ch)
			{
			case '1':
				choice = 1;
				break;
			case '2':
				choice = 2;
				break;
			case '3':
				choice = 3;
				break;
			case '4':
				choice = 4;
				break;
			case KEY_UP:
				if (highlight > 0)
				{
					highlight--;
				}
				break;
			case KEY_DOWN:
				if (highlight < 3)
				{
					highlight++;
				}
				break;
			case '\n':
				choice = highlight + 1;
				break;
			default:
				break;
			}

			if (choice != 0)
			{
				break;
			}
		}
		clear();
		return choice;
	}

	void RunGame()
	{
		clear();
		// Record the timestamp of the start of the game
		gameStartTime = std::time(NULL);
		// Initializing the game
		active.ChangeSize(LINES / 2, COLS / 2);
		stack.ChangeSize(LINES / 2, COLS / 2);
		control.GeneratePuyo(active, stack);
		control.ResetGame(active, stack);

		// Start the game
		bool isPaused = false;
		int delay = 0;

		while (!IsGameOver())
		{
			int ch = getch();
			// sの入力で一時停止
			if (ch == 's')
			{
				isPaused = !isPaused;
			}
			if (isPaused)
			{
				continue;
			}

			// Qの入力で終了
			if (ch == 'Q')
			{
				break;
			}

			if (control.LandingPuyo(active, stack))
			{
				control.VanishPuyo(active, stack);
				if (!control.LandFloating(active, stack))
				{
					control.GeneratePuyo(active, stack);
				}
			}
			else
			{
				if (control.CanMove(active, stack))
				{
					// 入力キーごとの処理
					switch (ch)
					{
					case KEY_LEFT:
						control.MoveLeft(active, stack);
						break;
					case KEY_RIGHT:
						control.MoveRight(active, stack);
						break;
					case KEY_DOWN:
						control.MoveDown(active, stack);
						break;
					case 'z':
						// ぷよ回転処理
						control.Rotate(active, stack);
						break;
					default:
						break;
					}
				}
			}
			// 処理速度調整のためのif文
			if (delay % waitCount == 0)
			{
				// ぷよ下に移動
				control.MoveDown(active, stack);
			}
			delay++;
			// 表示
			Display();
		}

		clear();
		ShowGameOverScreen();
	}

	bool IsGameOver()
	{
		if (CalculateGameDuration() > maxGameDuration)
		{
			return true;
		}

		// Check if the new Puyo generation location is occupied
		if (stack.GetValue(0, 5) != NONE || stack.GetValue(0, 6) != NONE)
		{
			return true;
		}

		return false;
	}

	// Calculate game duration in seconds
	int CalculateGameDuration()
	{
		std::time_t currentTime = std::time(NULL);
		return static_cast<int>(std::difftime(currentTime, gameStartTime));
	}

	void ShowGameOverScreen()
	{
		int score = stack.GetScore();
		clear();
		mvprintw(LINES / 2 - 5, COLS / 2 - 5, "Game Over");
		mvchgat(LINES / 2 - 5, COLS / 2 - 7, 13, A_REVERSE, 0, NULL);
		mvprintw(LINES / 2 - 2, COLS / 2 - 7, "Your Score: %d", score);
		mvprintw(LINES / 2, COLS / 2 - 26, "Do you want to save your score to scoreboard? (y/n): ");
		refresh();

		int ch;
		while (1)
		{
			ch = getch();
			if (ch == 'y' || ch == 'n')
			{
				break;
			}
		}

		if (ch == 'y')
		{
			mvprintw(LINES / 2 + 1, COLS / 2 - 8, "Enter your name: \n");
			refresh();

			char playerName[100];
			int playerNameMaxLength = 12;
			int playerNameLength = 0;

			int cursorX = COLS / 2 - 6; // Initial cursor position
			int cursorY = LINES / 2 + 3;
			curs_set(1);
			move(cursorY, cursorX);
			while (1)
			{
				int key = getch();

				if (key == '\n') // Press Enter to finish typing
				{
					if (playerNameLength == 0)
					{
						continue;
					}
					playerName[playerNameLength] = '\0';
					break;
				}
				else if (key == KEY_BACKSPACE && playerNameLength > 0)
				{
					playerNameLength--;
					mvaddch(cursorY, cursorX - 1, ' '); // Delete the previous character
					cursorX--;
					move(cursorY, cursorX); // Reset the cursor position
					refresh();
				}
				else if (key >= 32 && key <= 126 && playerNameLength < playerNameMaxLength) // 处理可见字符
				{
					playerName[playerNameLength] = key;
					mvaddch(cursorY, cursorX, key); // Display the entered characters
					cursorX++;
					playerNameLength++;
					move(cursorY, cursorX); // Reset the cursor position
					refresh();
				}

				refresh();
			}
			refresh();
			curs_set(0);

			// Save Plyer Info
			PlayerInfo playerInfo;
			playerInfo.name = playerName;
			playerInfo.score = score;
			playerInfoList.push_back(playerInfo);

			SavePlayerInfo("scoreboard.txt");
		}

		mvprintw(LINES / 2 + 5, COLS / 2 - 15, "Press 'q' to return to the main menu");
		refresh();
		while (1)
		{
			int ch = getch();
			if (ch == 'q')
			{
				clear();
				// exit game
				return;
			}
		}
	}

	void ShowScoreboard()
	{
		clear();

		mvprintw(3, COLS / 2 - 5, "Scoreboard");
		mvprintw(5, COLS / 2 - 10, "Name");
		mvprintw(5, COLS / 2 + 5, "Score");
		mvchgat(5, COLS / 2 - 15, 30, A_REVERSE, 0, NULL);

		int row = 6;

		for (std::vector<PlayerInfo>::const_iterator it = playerInfoList.begin(); it != playerInfoList.end(); ++it)
		{
			const PlayerInfo &player = *it;
			mvprintw(row, COLS / 2 - 10, "%s", player.name.c_str());
			mvprintw(row, COLS / 2 + 5, "%d", player.score);
			row++;
			if (row >= LINES - 2)
			{
				break;
			}
		}

		if (row >= 7)
		{
			mvprintw(6, COLS / 2 - 15, "1");
		}
		if (row >= 8)
		{
			mvprintw(7, COLS / 2 - 15, "2");
		}
		if (row >= 9)
		{
			mvprintw(8, COLS / 2 - 15, "3");
		}

		mvprintw(LINES - 1, 0, "Press 'q' to Quit");
		refresh();
		while (1)
		{
			int ch = getch();
			if (ch == 'q')
			{
				clear();
				return; // exit
			}
		}
	}

	// Return the highest score of all saved player data
	int GetTopScore()
	{
		if (!playerInfoList.empty())
		{
			return playerInfoList[0].score;
		}
		return 0; // Return 0 if the list is empty
	}

	void ShowSettingMenu()
	{
		while (1)
		{
			clear();

			int choice = 0;
			int highlight = 0;
			int ch;

			mvprintw(LINES / 2 - 2, COLS / 2 - 3, "Settings");
			mvprintw(LINES - 2, 0, "Press Up Down Enter or Number Key to Choose");
			mvprintw(LINES - 1, 0, "Press 'q' to Quit");
			refresh();
			while (1)
			{
				choice = 0;
				mvprintw(LINES / 2, COLS / 2 - 14, "1. Falling Speed of Puyo    ");
				mvprintw(LINES / 2 + 1, COLS / 2 - 14, "2. Max Game Duration        ");
				mvprintw(LINES / 2 + 2, COLS / 2 - 14, "3. Numbers of Color for Puyo");

				// Highlight the current option
				mvchgat(LINES / 2 + highlight, COLS / 2 - 14, 28, A_REVERSE, 0, NULL);

				ch = getch();

				switch (ch)
				{
				case '1':
					choice = 1;
					break;
				case '2':
					choice = 2;
					break;
				case '3':
					choice = 3;
					break;
				case KEY_UP:
					if (highlight > 0)
					{
						highlight--;
					}
					break;
				case KEY_DOWN:
					if (highlight < 2)
					{
						highlight++;
					}
					break;
				case '\n':
					choice = highlight + 1;
					break;
				case 'q':
					clear();
					return; // exit
				default:
					break;
				}

				if (choice != 0)
				{
					break;
				}
			}

			switch (choice)
			{
			case 1:
				ShowSetSpeedMenu();
				break;
			case 2:
				ShowSetMaxGameDurationMenu();
				break;
			case 3:
				ShowSetColorNumMenu();
			default:
				break;
			}
			clear();
		}
		return;
	}

	void ShowSetSpeedMenu()
	{
		clear();

		int choice = 0;
		int highlight = 0;
		int ch;

		mvprintw(LINES / 2 - 2, COLS / 2 - 12, "Set Falling Speed of Puyo");

		mvprintw(LINES - 2, 0, "Press Up Down Enter or Number Key to Choose");

		mvprintw(LINES - 1, 0, "Press 'q' to Quit");
		refresh();

		while (1)
		{
			mvprintw(LINES / 2, COLS / 2 - 5, " 1. Slow   ");
			mvprintw(LINES / 2 + 1, COLS / 2 - 5, " 2. Normal ");
			mvprintw(LINES / 2 + 2, COLS / 2 - 5, " 3. Fast   ");

			// Highlight the current option
			mvchgat(LINES / 2 + highlight, COLS / 2 - 5, 11, A_REVERSE, 0, NULL);

			ch = getch();

			switch (ch)
			{
			case '1':
				choice = 1;
				break;
			case '2':
				choice = 2;
				break;
			case '3':
				choice = 3;
				break;
			case KEY_UP:
				if (highlight > 0)
				{
					highlight--;
				}
				break;
			case KEY_DOWN:
				if (highlight < 2)
				{
					highlight++;
				}
				break;
			case '\n':
				choice = highlight + 1;
				break;
			case 'q':
				clear();
				return; // exit
			default:
				break;
			}

			if (choice != 0)
			{
				break;
			}
		}
		SetwaitCount(choice);
		clear();

		return;
	}

	void SetwaitCount(int choice)
	{
		switch (choice)
		{
		case 1:
			waitCount = 40000;
			break;
		case 2:
			waitCount = 20000;
			break;
		case 3:
			waitCount = 10000;
			break;
		default:
			break;
		}
		return;
	}

	void ShowSetMaxGameDurationMenu()
	{
		clear();

		int choice = 0;
		int highlight = 0;
		int ch;

		mvprintw(LINES / 2 - 2, COLS / 2 - 10, "Set Max Game Duration");

		mvprintw(LINES - 2, 0, "Press Up Down Enter or Number Key to Choose");

		mvprintw(LINES - 1, 0, "Press 'q' to Quit");
		refresh();

		while (1)
		{
			mvprintw(LINES / 2, COLS / 2 - 8, " 1. 300  Seconds ");
			mvprintw(LINES / 2 + 1, COLS / 2 - 8, " 2. 600  Seconds ");
			mvprintw(LINES / 2 + 2, COLS / 2 - 8, " 3. 1200 Seconds ");

			// Highlight the current option
			mvchgat(LINES / 2 + highlight, COLS / 2 - 8, 17, A_REVERSE, 0, NULL);

			ch = getch();

			switch (ch)
			{
			case '1':
				choice = 1;
				break;
			case '2':
				choice = 2;
				break;
			case '3':
				choice = 3;
				break;
			case KEY_UP:
				if (highlight > 0)
				{
					highlight--;
				}
				break;
			case KEY_DOWN:
				if (highlight < 2)
				{
					highlight++;
				}
				break;
			case '\n':
				choice = highlight + 1;
				break;
			case 'q':
				clear();
				return; // exit
			default:
				break;
			}

			if (choice != 0)
			{
				break;
			}
		}
		SetmaxGameDuration(choice);
		clear();

		return;
	}

	void SetmaxGameDuration(int choice)
	{
		switch (choice)
		{
		case 1:
			maxGameDuration = 300;
			break;
		case 2:
			maxGameDuration = 600;
			break;
		case 3:
			maxGameDuration = 1200;
			break;
		default:
			break;
		}
		return;
	}

	void ShowSetColorNumMenu()
	{
		clear();

		int choice = 0;
		int highlight = 0;
		int ch;

		mvprintw(LINES / 2 - 2, COLS / 2 - 15, "Set the Number of Colors for Puyo");

		mvprintw(LINES - 2, 0, "Press Up Down Enter or Number Key to Choose");

		mvprintw(LINES - 1, 0, "Press 'q' to Quit");
		refresh();

		while (1)
		{
			mvprintw(LINES / 2, COLS / 2 - 6, " 1. 4 Colors ");
			mvprintw(LINES / 2 + 1, COLS / 2 - 6, " 2. 5 Colors ");

			// Highlight the current option
			mvchgat(LINES / 2 + highlight, COLS / 2 - 6, 13, A_REVERSE, 0, NULL);

			ch = getch();

			switch (ch)
			{
			case '1':
				choice = 1;
				break;
			case '2':
				choice = 2;
				break;
			case KEY_UP:
				if (highlight > 0)
				{
					highlight--;
				}
				break;
			case KEY_DOWN:
				if (highlight < 1)
				{
					highlight++;
				}
				break;
			case '\n':
				choice = highlight + 1;
				break;
			case 'q':
				clear();
				return; // exit
			default:
				break;
			}

			if (choice != 0)
			{
				break;
			}
		}
		control.SetColorNum(choice + 3);
		clear();

		return;
	}

	void Display()
	{
		// 文字の色と背景の色のペアを初期化する
		init_pair(0, COLOR_WHITE, COLOR_BLACK);
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_BLUE, COLOR_BLACK);
		init_pair(3, COLOR_GREEN, COLOR_BLACK);
		init_pair(4, COLOR_YELLOW, COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_WHITE);
		init_pair(6, COLOR_CYAN, COLOR_BLACK);
		init_pair(7, COLOR_MAGENTA, COLOR_BLACK);

		// ぷよ表示
		for (int y = 0; y < active.GetLine(); y++)
		{
			for (int x = 0; x < active.GetColumn(); x++)
			{
				if (active.GetValue(y, x) != NONE)
				{
					switch (active.GetValue(y, x))
					{
					case RED:
						attrset(COLOR_PAIR(1));
						mvaddch(y, x, 'R');
						break;
					case BLUE:
						attrset(COLOR_PAIR(2));
						mvaddch(y, x, 'B');
						break;
					case GREEN:
						attrset(COLOR_PAIR(3));
						mvaddch(y, x, 'G');
						break;
					case YELLOW:
						attrset(COLOR_PAIR(4));
						mvaddch(y, x, 'Y');
						break;
					case PURPLE:
						attrset(COLOR_PAIR(7));
						mvaddch(y, x, 'P');
						break;
					default:
						break;
					}
				}
				else
				{
					switch (stack.GetValue(y, x))
					{
					case NONE:
						attrset(COLOR_PAIR(0));
						mvaddch(y, x, '.');
						break;
					case RED:
						attrset(COLOR_PAIR(1));
						mvaddch(y, x, 'R');
						break;
					case BLUE:
						attrset(COLOR_PAIR(2));
						mvaddch(y, x, 'B');
						break;
					case GREEN:
						attrset(COLOR_PAIR(3));
						mvaddch(y, x, 'G');
						break;
					case YELLOW:
						attrset(COLOR_PAIR(4));
						mvaddch(y, x, 'Y');
						break;
					case PURPLE:
						attrset(COLOR_PAIR(7));
						mvaddch(y, x, 'P');
						break;
					default:
						mvaddch(y, x, '?');
						break;
					}
				}
			}
		}

		// Display NextPuyo
		for (int y = 1; y < 3; y++)
		{
			for (int x = 0; x < 2; x++)
			{

				switch (active.GetNextPuyoValue(y, x))
				{
				case RED:
					attrset(COLOR_PAIR(1));
					mvaddch(y + 5, x + COLS - 23, 'R');
					break;
				case BLUE:
					attrset(COLOR_PAIR(2));
					mvaddch(y + 5, x + COLS - 23, 'B');
					break;
				case GREEN:
					attrset(COLOR_PAIR(3));
					mvaddch(y + 5, x + COLS - 23, 'G');
					break;
				case YELLOW:
					attrset(COLOR_PAIR(4));
					mvaddch(y + 5, x + COLS - 23, 'Y');
					break;
				case PURPLE:
					attrset(COLOR_PAIR(7));
					mvaddch(y + 5, x + COLS - 23, 'P');
					break;
				case NONE:
					attrset(COLOR_PAIR(0));
					mvaddch(y + 5, x + COLS - 23, '.');
				default:
					break;
				}
			}
		}

		// 情報表示
		int count = active.CountPuyo() + stack.CountPuyo();
		int score = stack.GetScore();
		int topscore = GetTopScore();
		int gameDuration = CalculateGameDuration();

		char msg[256];
		sprintf(msg, "Field: %d x %d, Puyo number: %03d", active.GetLine(), active.GetColumn(), count);
		attrset(COLOR_PAIR(0));
		mvaddstr(2, COLS - 35, msg);

		char scoreMsg[256];
		sprintf(scoreMsg, "Score: %d", score);
		attrset(COLOR_PAIR(6));
		mvaddstr(3, COLS - 35, scoreMsg);

		if (score > topscore)
		{
			attrset(COLOR_PAIR(5));
			mvaddstr(3, COLS - 15, "HIGH SCORE");
		}

		attrset(COLOR_PAIR(0));
		mvprintw(5, COLS - 15, "Max Chain: %d", control.GetMaxChain());

		char timer[256];
		sprintf(timer, "Game Time: %ds / %ds", gameDuration, maxGameDuration);
		attrset(COLOR_PAIR(0));
		mvaddstr(LINES / 2 + 1, 2, timer);

		mvprintw(6, COLS - 35, "Next Puyo: ");

		mvprintw(LINES - 1, 0, "Q: Quit");
		mvprintw(LINES - 2, 0, "s: Pause/Resume");

		mvprintw(LINES / 2 + 1, COLS - 35, "Use the following keys to play:");
		mvprintw(LINES / 2 + 3, COLS - 30, "Arrow Left: Move Left");
		mvprintw(LINES / 2 + 4, COLS - 30, "Arrow Right: Move Right");
		mvprintw(LINES / 2 + 5, COLS - 30, "Arrow Down: Move Down");
		mvprintw(LINES / 2 + 6, COLS - 30, "z: Rotate");
		refresh();
	}
};

int main()
{
	PuyoGame game;

	game.Run();

	return 0;
}