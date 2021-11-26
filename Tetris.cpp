#include "Tetris.h"

default_random_engine RandomEngine(random_device{}());
gamezone_t gamezone = { 0 };
ScoreInfo score;
BlockInfo current, successor;
system_clock::time_point start_time;
ButtonManager mainBM({2 * CELL, 2 * CELL}), scoreBM({2 * CELL, 2 * CELL});
const wchar_t info[] =
LR"(Tetris ����˹���� By Yaossg
���� C++17 ��дʹ�� EasyX �⣬�漰�� MFC)";
const wchar_t font[] = L"����";


void newInterface(int width, int height)
{
	cleardevice();
	Resize(NULL, CELL * width, CELL * height);
	setorigin(0, 0);
}

void initEnviroment()
{
	newInterface(30, 24);
	setorigin(CELL * 8, CELL);
	//������
	rectangle(-1, -1, CELL * MWIDTH, CELL * MHEIGHT);
	//Ԥ����
	rectangle(CELL * (MWIDTH + 1) - 1, -1,
		CELL * (MWIDTH + AWIDTH + 1), CELL * AHEIGHT);
	RECT r1 = { 
		CELL * MWIDTH + CELL, 
		CELL * AHEIGHT + CELL * 4,
		CELL * MWIDTH + CELL * 8,
		CELL * AHEIGHT + CELL * 6 
	};
	drawtext(L"��ESC�˳�\n��P��ͼ", &r1, DT_LEFT | DT_VCENTER);
}

void drawBlocks(BlockInfo const& block)
{
	setfillcolor(block.color());
	setlinecolor(Colors::WHITE);
	FORBLOCK(block, i, j)
		fillrectangle(AS_BLOCK(block.x + j, block.y + i));
}

void eraseBlocks(BlockInfo const& block)
{
	FORBLOCK(block, i, j)
		clearrectangle(AS_BLOCK(block.x + j, block.y + i));
}

void drawScore()
{
	wstring ws = static_cast<wstringstream&>(wstringstream()
		<< L"����:" << score.score << endl
		<< L"��ǰ�Ѷ�:" << score.difficulty).str();
	settextstyle(CELL, 0, font);
	RECT r = { CELL * MWIDTH + CELL, CELL * AHEIGHT + CELL,
		CELL * MWIDTH + CELL * 16, CELL * AHEIGHT + CELL * 8 };
	drawtext(ws.c_str(), &r, DT_LEFT | DT_VCENTER);
}

bool tryFall()
{
	system_clock::time_point stop_time = system_clock::now();
	unsigned long long dur_time =
		duration_cast<milliseconds>(stop_time - start_time).count();
	if (dur_time > 1500u / (1 + score.difficulty * 0.365))
	{
		start_time = stop_time;
		return true;
	}
	return false;
}

void moveBlock(BlockInfo (BlockInfo::* pmf)() const)
{
	BlockInfo temp = (current.*pmf)();
	if (movableAndRotatable(temp) && fallable(temp))
	{
		BatchDrawer drawer;
		eraseBlocks(current);
		drawBlocks(current = temp);
	}
}

bool fallDown()
{
	BlockInfo temp = current.downFall();
	BatchDrawer drawer;
	if (fallable(temp))
	{
		eraseBlocks(current);
		drawBlocks(current = temp);
		return true;
	}
	else
	{
		setblock(current);
		clearLine();
		return spawnblocks();
	}
}

bool movableAndRotatable(BlockInfo const& block)
{
	FORBLOCK(block, i, j)
		if ((block.x + j >= MWIDTH) || (block.x + j < 0))
			return false;
	return true;
}

bool fallable(BlockInfo const& block)
{
	FORBLOCK(block, i, j)
		if (block.y + i >= MHEIGHT || gamezone[block.y + i][block.x + j])
			return false;
	return true;
}

bool spawnblocks()
{
	BatchDrawer drawer;
	eraseBlocks(successor);
	current.t = successor.t;
	current.s = successor.s;
	current.x = (MWIDTH - 4) / 2;
	current.y = 0;
	if (blockfull(current))
		return false;
	drawBlocks(current);
	randomblocks();
	drawBlocks(successor);
	return true;
}

void randomblocks()
{
	using dist = uniform_int_distribution<int>;
	successor.t = dist{ 0, 6 }(RandomEngine);
	successor.s = dist{ 0, 3 }(RandomEngine);
	successor.x = MWIDTH + 1;
	successor.y = 0;
}

void setblock(BlockInfo const& block)
{
	FORBLOCK(block, i, j)
		gamezone[current.y + i][current.x + j] = current.t + 1;
}

void clearLine()
{
	int i = MHEIGHT - 1, count = 0;
	while (i >= 0)
	{
		if (find(gamezone[i], gamezone[i + 1], 0) == gamezone[i + 1])
		{
			IMAGE img;
			setfillcolor(Colors::BLACK);
			solidrectangle(0, CELL * i, CELL * MWIDTH - 1, CELL * (i + 1) - 1);
			for (int a = i; a > 0; a--)
				for (int b = 0; b < MWIDTH; b++)
					gamezone[a][b] = gamezone[a - 1][b];
			getimage(&img, 0, 0, CELL * MWIDTH, CELL * i);
			putimage(0, CELL, &img);
			++count;
		}
		else --i;
	}
	score.clearLine(count);
	drawScore();
}

bool blockfull(BlockInfo const& block)
{
	for (int i = 0; i < AHEIGHT; i++)
		for (int j = 3; j < 3 + AWIDTH; j++)
			if (gamezone[i][j])
				return true;
	return false;
}

int getKey()
{
	if (tryFall())return DOWN;
	if (_kbhit())
	{
		switch (_getch())
		{
		case DIR:
			switch (_getch())
			{
			case UP:	return UP;
			case DOWN:	return DOWN;
			case LEFT:	return LEFT;
			case RIGHT:	return RIGHT;
			}break;
		case 'W': case 'w': return UP;
		case 'A': case 'a': return LEFT;
		case 'S': case 's': return DOWN;
		case 'D': case 'd': return RIGHT;
		case 'P': case 'p': return SCREENSHOT;
		case ESC:			return ESC;
		}
	}
	return DEF;
}

int execute()
{
	switch (getKey())
	{
	case UP:
		moveBlock(&BlockInfo::upRotate);
		break;
	case LEFT:
		moveBlock(&BlockInfo::leftMove);
		break;
	case RIGHT:
		moveBlock(&BlockInfo::rightMove);
		break;
	case DOWN:
		if (!fallDown())
			return LOST;
		break;
	case ESC:
		switch (MessageBox(GetHWnd(), L"\"��\":���沢�˳�\n\"��\":ֱ���˳�\"ȡ��:\"���˳�", 
			L"��ȷ��Ҫ�˳���������", MB_YESNOCANCEL | MB_ICONWARNING))
		{
		case IDYES:
			saveGame();
		case IDNO:
			return ESC;
		default:
		case IDCANCEL:
			;
		}break;
	case SCREENSHOT: screenshot();  break;
	}
	return DEF;
}

void initButtons()
{
	mainBM.add({ CELL * 2, CELL, CELL * 10, CELL * 3,
		Colors::RED, L"����Ϸ", newGame, true });
	mainBM.add({ CELL * 2, CELL * 4, CELL * 10, CELL * 6,
		Colors::ORANGE, L"������Ϸ", oldGame, true });
	mainBM.add({ CELL * 2, CELL * 7, CELL * 10, CELL * 9,
		 Colors::GREEN, L"���а�", showScoreList, true });
	mainBM.add({ CELL * 2, CELL * 10, CELL * 10, CELL * 12,
		Colors::PURPLE, L"����", showInfo, false });
	mainBM.add({ CELL * 2, CELL * 13, CELL * 10, CELL * 15,
		Colors::BLUE, L"�˳�", tryExit, false });

	scoreBM.add({ CELL * 3, CELL * 16, CELL * 8, CELL * 18,
		Colors::VOILET, L"����", [] {}, true });
}

void Tetris()
{
	initButtons();
	initgraph(CELL * 16, CELL * 20, EW_NOCLOSE);
	setbkcolor(Colors::BLACK);
	while (true)
	{
		newInterface(16, 20);
		setlinecolor(Colors::WHITE);
		clearrectangle(0, 0, CELL * 16, CELL * 20);
		RECT r = { 0, 0, CELL * 16, CELL * 2 - 1 };
		settextstyle(CELL * 2, 0, font);
		drawtext(L"����˹����", &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		setorigin(CELL * 2, CELL * 2);
		rectangle(-1, -1, CELL * 12, CELL * 16);
		mainBM.draw();
		do
			while( !mainBM.exec());
		while (!mainBM.repaint);
	}
}

void showInfo()
{
	MessageBox(GetHWnd(), info, L"����", MB_OK | MB_ICONINFORMATION);
}

void tryExit()
{
	if (MessageBox(GetHWnd(), L"��ȷ��Ҫ�˳���?", L"�˳�", MB_YESNO | MB_ICONWARNING) == IDYES)
	{
		closegraph();
		exit(EXIT_SUCCESS);
	}
}

void newGame()
{
	start_time = system_clock::now();
	int ret;
	score.name.clear();
	do
	{
		score.difficulty = 1;
		initEnviroment();
		memset(gamezone, 0, sizeof gamezone);
		drawScore();
		randomblocks();
		spawnblocks();
		while ((ret = execute()) == DEF);
	} while (ret == LOST && gameLost() == IDYES);
}

void oldGame()
{
	askName(L"������Ϸ");
	if (!fs::exists(score.name))
	{
		MessageBox(GetHWnd(), L"û�п��õĴ浵", L"���ش浵", MB_OK);
		return;
	}
	start_time = system_clock::now();
	loadGame();
	fs::remove(score.name);
	int ret;
	while ((ret = execute()) == DEF);
	if (ret == LOST && gameLost() == IDYES)
		newGame();
}

int gameLost()
{
	if(score.name.empty())askName(L"��Ϸ����");
	putScoreList();
	return MessageBox(GetHWnd(), static_cast<wstringstream&>(
		wstringstream() << L"����:" << score.score
		<< endl << L"��������һ����"
		).str().c_str(), L"��Ϸ����", MB_YESNO | MB_ICONWARNING);
}

void initScoreList()
{
	wofstream file("ScoreList.txt");
	for (int i = 0; i < SCORELISTSIZE; i++)
		file << L"- 0\n";
}

void putScoreList()
{
	ScoreList scorelist;
	getScoreList(scorelist);
	wofstream file("ScoreList.txt");
	scorelist.insert({ score.score, score.name });
	scorelist.erase(--scorelist.end());
	for (ScoreListElement const& e : scorelist) 
	{
		file << e.second << L' ' << e.first;
	}
}

void getScoreList(ScoreList& scorelist)
{
	wifstream file("ScoreList.txt");
	if (file.fail())
	{
		initScoreList();
		file.open("ScoreList.txt");
	}
	for (int i = 0; i < SCORELISTSIZE; i++) 
	{
		wstring name; int score; file >> name >> score;
		scorelist.insert({ score, name });
	}
}

void showScoreList()
{
	newInterface(16, 21);
	setfillcolor(Colors::WHITE);
	setlinecolor(Colors::WHITE);
	RECT r = { 0, 0, CELL * 16, CELL * 2 - 1 };
	settextstyle(CELL * 2, 0, font);
	drawtext(L"���а�", &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	setorigin(CELL * 2, CELL * 2);
	rectangle(-1, -1, CELL * 12, CELL * 15);
	ScoreList scorelist;
	getScoreList(scorelist);
	r.right = CELL * 12;
	r.bottom = CELL;
	settextstyle(CELL, 0, NULL);
	int i = 1;
	for (ScoreListElement const& e : scorelist) 
	{
		drawtext((
			wstringstream() << left
			<< setw(3) << i++
			<< setw(10) << e.second
			<< setw(8) << e.first
			).str().c_str(), &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		r.bottom += CELL * 3;
	}
	scoreBM.draw();
	while (!scoreBM.exec());
}

void screenshot() 
{
	time_t t = time(nullptr);
	tm* tp = localtime(&t);
	saveimage((wstringstream()
		<< L"screenshot" << 1900 + tp->tm_year
		<< setw(2) << 1 + tp->tm_mon
		<< tp->tm_mday << tp->tm_hour
		<< tp->tm_min << tp->tm_sec
		<< L".png").str().c_str());
	MessageBox(GetHWnd(), L"��ͼ�ѱ���", L"��ͼ", MB_OK);
}

void saveGame()
{
	if(score.name.empty()) askName(L"������Ϸ");
	fs::remove(score.name);
	SaveBuf buf;
	memcpy(buf.gamezone, gamezone, sizeof gamezone);
	memcpy(&buf.current, &current, sizeof current);
	buf.successor.t = successor.t;
	buf.successor.s = successor.s;
	buf.score.score = score.score;
	buf.score.difficulty = score.difficulty;
	ofstream file(score.name, ios::binary);
	file.write(reinterpret_cast<char*>(&buf), sizeof buf);
}

void loadGame()
{
	initEnviroment();

	ifstream file(score.name, ios::binary);
	SaveBuf buf;
	file.read(reinterpret_cast<char*>(&buf), sizeof buf);

	memcpy(gamezone, buf.gamezone, sizeof gamezone);
	memcpy(&current, &buf.current, sizeof current);
	successor.t = buf.successor.t;
	successor.s = buf.successor.s;
	score.score = buf.score.score;
	score.difficulty = buf.score.difficulty;

	successor.x = MWIDTH + 1;
	successor.y = 0;

	setlinecolor(Colors::WHITE);
	for (int i = 0; i < MHEIGHT; i++)
	{
		for (int j = 0; j < MWIDTH; j++)
		{
			if (gamezone[i][j]) 
			{
				setfillcolor(TetrisInfo[gamezone[i][j] - 1].color);
				fillrectangle(AS_BLOCK(j, i));
			}
		}
	}
	drawScore();
	drawBlocks(current);
	drawBlocks(successor);
}

void askName(wstring title)
{
	wchar_t buf[64];
	do
	{
		InputBox(buf, sizeof buf / sizeof *buf, L"�������ǳƣ�(������ 128 �ַ���", title.c_str());
		score.name = buf;
		if (score.name.empty())
			MessageBox(GetHWnd(), L"��������ǳ�Ϊ�գ�����������",
				L"�ǳƲ���Ϊ��", MB_OK);
		if (score.name.length() > 16)
			MessageBox(GetHWnd(), L"��������ǳƹ���������������",
				L"�ǳƹ���", MB_OK);
	} while (score.name.length() > 128 || score.name.empty());
}
