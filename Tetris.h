//ͷ�ļ�����
//non-standard libaray
#include <easyx.h>
#include <graphics.h>
#include <conio.h>
#include <windows.h>
//standard libaray
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <random>
#include <vector>
#include <chrono>
#include <filesystem>
#include <set>
using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;

//�Ƴ� easyx ��ɫ����
#undef	BLACK
#undef	BLUE
#undef	GREEN
#undef	CYAN
#undef	RED
#undef	MAGENTA
#undef	BROWN
#undef	LIGHTGRAY
#undef	DARKGRAY
#undef	LIGHTBLUE
#undef	LIGHTGREEN
#undef	LIGHTCYAN
#undef	LIGHTRED
#undef	LIGHTMAGENTA
#undef	YELLOW
#undef	WHITE

// SVG Colors
namespace Colors {
	constexpr COLORREF WHITE = BGR(0xFFFFFF);
	constexpr COLORREF BLACK = BGR(0x000000);
	constexpr COLORREF RED = BGR(0xFF0000);
	constexpr COLORREF ORANGE = BGR(0xFFA500);
	constexpr COLORREF YELLOW = BGR(0xFFFF00);
	constexpr COLORREF GREEN = BGR(0x008000);
	constexpr COLORREF CYAN = BGR(0x00FFFF);
	constexpr COLORREF BLUE = BGR(0x0000FF);
	constexpr COLORREF PURPLE = BGR(0x800080);
	constexpr COLORREF VOILET = BGR(0xEE82EE);
}

//�궨�岿��
#define CELL 20

#define MWIDTH 10
#define MHEIGHT 22

typedef BYTE gamezone_t[MHEIGHT][MWIDTH];

#define AWIDTH 4
#define AHEIGHT 4
/*
ɨ�����������ֽڣ���ASCII���й�ϵ��ASCII����ж�Ӧ���ַ�
��ɨ����ĵ�һ���ֽھ���ASCII�룬�ڶ����ֽھ���0
����	��һ���ֽ�	�ڶ����ֽ�
������		224			72
������		224			80
������		224			75
������		224			77
*/
#define DIR 224
#define UP 72
#define DOWN 80
#define LEFT 75
#define RIGHT 77

#define ESC 27
#define SCREENSHOT 233

#define LOST 2
#define DEF -1

#define SCORELISTSIZE 10

#define AS_BLOCK(x, y) (CELL * (x)), (CELL * (y)), \
	(CELL * ((x) + 1) - 1), (CELL * ((y) + 1) - 1)

#define FORBLOCK(block, i, j) \
	for (WORD __n = block.shape(), i = 0; i < AHEIGHT; ++i) \
		for (WORD j = 0; j < AWIDTH; ++j, __n <<= 1) \
			if (0x8000 & __n)
	

const struct
{
	WORD shape[4];
	COLORREF color;
} TetrisInfo[7] = {
	{ 0x0F00, 0x4444, 0x0F00, 0x4444, Colors::RED },	//I����
	{ 0x0660, 0x0660, 0x0660, 0x0660, Colors::ORANGE },	//����
	{ 0x4460, 0x02E0, 0x0622, 0x0740, Colors::YELLOW },	//��L����
	{ 0x2260, 0x0E20, 0x0644, 0x0470, Colors::GREEN },	//��L����
	{ 0x0C60, 0x2640, 0x0C60, 0x2640, Colors::CYAN },	//��Z����
	{ 0x0360, 0x4620, 0x0360, 0x4620, Colors::BLUE },	//��Z����
	{ 0x04E0, 0x4C40, 0x0E40, 0x4640, Colors::PURPLE },	//T����
};

//�ṹ����������
struct BlockInfo
{
	int x, y, t, s;

	BlockInfo leftMove() const
	{
		return{ x - 1, y, t, s };
	}
	BlockInfo rightMove() const
	{
		return{ x + 1, y, t, s };
	}
	BlockInfo upRotate() const
	{
		return{ x, y, t, (s + 1) % 4 };
	}
	BlockInfo downFall() const
	{
		return{ x, y + 1, t, s };
	}
	WORD shape() const 
	{
		return TetrisInfo[t].shape[s];
	}
	COLORREF color() const 
	{
		return TetrisInfo[t].color;
	}
};

struct ScoreInfo
{
	ScoreInfo() : score(0), difficulty(1), name(L"-") {}
	int score, difficulty;
	wstring name;
	void clearLine(int times)
	{
		if (times > 0)
		{
			int count = difficulty * times;
			for (int i = 1; i < times; i++)
				count += (difficulty * i + 1) / 2;
			if (score > 3 * difficulty * difficulty / times) {
				++difficulty;
			}
			score += count;
		}
	}
};

using ScoreListElement = pair<int, wstring>;
using ScoreList = multiset<ScoreListElement, greater<ScoreListElement>>;

struct SaveBuf {
	gamezone_t gamezone;
	struct {
		int x, y, t, s;
	} current;
	struct {
		int t, s;
	} successor;
	struct {
		int score, difficulty;
	} score;
};


struct BatchDrawer
{
	BatchDrawer()
	{
		BeginBatchDraw();
	}
	~BatchDrawer()
	{
		EndBatchDraw();
	}
};

struct Button
{
	void draw() const
	{
		setfillcolor(color);
		COLORREF bgcolor = getbkcolor();
		setbkcolor(color);
		fillrectangle(left, top, right, bottom);
		RECT r = { left, top, right, bottom };
		settextstyle(CELL, 0, NULL);
		drawtext(text.c_str(), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		setbkcolor(bgcolor);
	}
	int left, top, right, bottom;
	COLORREF color;
	wstring text;
	void (*func)();
	bool repaint;
};

struct ButtonManager
{
	void add(Button const& button)
	{
		buttons.push_back(button);
	}
	void draw() const
	{
		BatchDrawer drawer;
		for (Button button : buttons)
			button.draw();
	}
	bool exec()
	{
		if (MouseHit())
		{
			MOUSEMSG msg = GetMouseMsg();
			switch (msg.uMsg)
			{
			case WM_LBUTTONUP:
			{
				for (Button button : buttons)
				{
					if (button.left <= msg.x - CELL * 2
						&& button.right >= msg.x - CELL * 2
						&& button.top <= msg.y - CELL * 2
						&& button.bottom >= msg.y - CELL * 2)
					{
						FlushMouseMsgBuffer();
						button.func();
						repaint = button.repaint;
						return true;
					}
				}break;
			}
			}
		}
		return false;
	}
	bool repaint = true;
private:
	std::vector<Button> buttons;
};

//��ʼ��
void newInterface(int width, int height);
void initEnviroment();

//���ƺ���
void drawBlocks(BlockInfo const& block);
void eraseBlocks(BlockInfo const& block);
void drawScore();

//����˹������ƶ�ϵ��
void moveBlock(BlockInfo(BlockInfo::* pmf)() const);
bool fallDown();

//�ж��ܷ��ƶ�
bool movableAndRotatable(BlockInfo const& block);
bool fallable(BlockInfo const& block);

//���ɷ���
bool spawnblocks();
void randomblocks();

//���ú��������
void setblock(BlockInfo const& block);
void clearLine();
bool blockfull(BlockInfo const& block);

//��Ϸ��ʼ
void newGame();
void oldGame();
int gameLost();

//���̿���
bool tryFall();
int getKey();
int execute();

//������
void initButtons();
void Tetris();
void showInfo();
void tryExit();

//�߷ְ�
void initScoreList();
void putScoreList();
void getScoreList(ScoreList& sorcelist);
void showScoreList();

//��Ϸ���������
void screenshot();

void saveGame();
void loadGame();
void askName(wstring title);
