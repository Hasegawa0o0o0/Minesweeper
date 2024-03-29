//
// Minesweeper
//

#include "stdafx.h"
#include <iostream>
#include <conio.h>
#include <string>
#include <time.h>
#include <iomanip>
#include <windows.h>

using namespace std;

struct Vector2
{
	int x;
	int y;
};

//----------------------------------------
// コンソール色変更関数（背景色・文字色)
// 0x00ACなら背景色をA, 文字色をCに変更
//----------------------------------------
void changeColor(int color)
{
	HANDLE hStdout;
	// コンソールスクリーンバッファのハンドルを取得
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	// プロンプトの文字の色を変更
	SetConsoleTextAttribute(hStdout, color);
}

// マスクラス(基本)
class Mass
{
protected:
	Vector2 pos;
	bool isOpen;
	bool isFlag;
public:
	Mass(const Vector2 p = {}) : pos(p), isOpen(false), isFlag(false) {}
	virtual void addNum(const int num = 0) {}
	virtual int getNum()const { return 0; }
	virtual string getType()const { return "Mass"; }
	void setFlag(const bool flag = false) { isFlag = flag; }
	bool getFlag()const { return isFlag; }
	void open() { isOpen = true; }
	bool getIsOpen()const { return isOpen; }
};
//-----------------------------------------------------------
// 安全マスクラス(派生)
class FreeMass : public Mass
{
	int aroundBombNum;
public:
	FreeMass(const Vector2 pos = {}) : aroundBombNum(0), Mass(pos) {}
	void addNum(const int num = 0) { aroundBombNum += num; }
	int getNum()const { return aroundBombNum; }
	string getType()const { return "FreeMass"; }
};
//-----------------------------------------------------------
// 爆弾マスクラス(派生)
class BombMass : public Mass
{
public:
	BombMass(const Vector2 pos = {}) : Mass(pos) {}
	string getType()const { return "BombMass"; }
};
//-----------------------------------------------------------
// ゲームボードクラス
class GameBoard
{
	Mass* massP[100][100] = {};
	Vector2 selectPos;
	const int cBombNum;
	const int cOneSideNum;
	int flagCntNum;
	bool isClear;
public:
	GameBoard(const int bombN = 40, const int oneSideN = 16) :cBombNum(bombN), cOneSideNum(oneSideN), flagCntNum(0), isClear(true)
	{
		// 選択位置の初期化
		selectPos.x = 0;
		selectPos.y = 0;
		// 爆弾マスの配置と生成
		for (int i = 0; i < cBombNum; ++i)
		{
			int bombX = rand() % cOneSideNum;
			int bombY = rand() % cOneSideNum;
			// すでに生成されていたときは右にずらす
			if (massP[bombY][bombX])
			{
				for (; massP[bombY][bombX] != NULL; ++bombY)
				{
					for (; massP[bombY][bombX] != NULL && bombX < cOneSideNum; ++bombX) {}
					if (bombX >= cOneSideNum)
					{
						bombX = 0;
					}
					if (massP[bombY][bombX] == NULL)
					{
						break;
					}
					if (bombY + 1 >= cOneSideNum)
					{
						bombY = -1;
					}
				}
			}
			Vector2 insertPos = { bombX, bombY };
			massP[bombY][bombX] = new BombMass(insertPos);
		}
		// 安全マスを生成
		for (int y = 0; y < cOneSideNum; ++y)
		{
			for (int x = 0; x < cOneSideNum; ++x)
			{
				if (massP[y][x]) { continue; }
				// 生成位置
				Vector2 insertPos = { x,y };
				// マスの生成
				massP[y][x] = new FreeMass(insertPos);
			}
		}
		// 安全マスが周りの爆弾を数える
		for (int y = 0; y < cOneSideNum; ++y)
		{
			for (int x = 0; x < cOneSideNum; ++x)
			{
				// 確認位置
				Vector2 checkPos = { x,y };
				// 爆弾マスだったらとばす
				if (isBombMass(checkPos)) { continue; }
				// 周囲の爆弾の数をカウント
				for (int smallY = -1; smallY < 2; ++smallY)
				{
					// yが範囲外だったらとばす
					if (checkPos.y + smallY < 0 || checkPos.y + smallY > cOneSideNum - 1) { continue; }
					for (int smallX = -1; smallX < 2; ++smallX)
					{
						if (smallY == 0 && smallX == 0) { continue; }
						// xが範囲外だったらとばす
						if (checkPos.x + smallX < 0 || checkPos.x + smallX > cOneSideNum - 1) { continue; }
						if (isBombMass(checkPos.y + smallY, checkPos.x + smallX)) { massP[y][x]->addNum(1); }
					}
				}
			}
		}
	}
	//-------------------------------------------------------
	// 開いていないマスを数える
	int countCloseMass()
	{
		int closeMassCnt = 0;
		for (int y = 0; y < cOneSideNum; ++y)
		{
			for (int x = 0; x < cOneSideNum; ++x)
			{
				if (!massP[y][x]->getIsOpen())
				{
					++closeMassCnt;
				}
			}
		}
		return closeMassCnt;
	}
	//-------------------------------------------------------
	// マスを開ける
	void openMass(const Vector2 openPos = {}, const bool isFirst = false)
	{
		if (openPos.y < 0 || openPos.y > cOneSideNum - 1 || openPos.x < 0 || openPos.x > cOneSideNum - 1)
		{
			return;
		}
		if (massP[openPos.y][openPos.x]->getFlag())
		{
			return;
		}
		// 爆弾マスであれば爆弾マスをすべて開ける
		if (isBombMass(openPos) && isFirst)
		{
			for (int smallY = 0; smallY < cOneSideNum; ++smallY)
			{
				for (int smallX = 0; smallX < cOneSideNum; ++smallX)
				{
					if (!isBombMass(smallY, smallX)) { continue; }
					massP[smallY][smallX]->open();
				}
			}
			isClear = false;
			return;
		}
		massP[openPos.y][openPos.x]->open();
		// 周りに爆弾があるマスだったら終了
		if (massP[openPos.y][openPos.x]->getNum() != 0)
		{
			return;
		}
		else
		{
			// 周りの8つを開く
			for (int smallY = -1; smallY < 2; ++smallY)
			{
				for (int smallX = -1; smallX < 2; ++smallX)
				{
					// 画面外、またはもう開いていれば開かない
					if (openPos.y + smallY < 0 || openPos.y + smallY > cOneSideNum - 1 || openPos.x + smallX < 0 || openPos.x + smallX > cOneSideNum - 1) { continue; }
					if (massP[openPos.y + smallY][openPos.x + smallX]->getIsOpen()) { continue; }
					openMass(openPos.y + smallY, openPos.x + smallX, false);
				}
			}
		}
	}
	void openMass(const int y = 0, const int x = 0, const bool isFirst = false)
	{
		if (y < 0 || y > cOneSideNum - 1 || x < 0 || x > cOneSideNum - 1)
		{
			return;
		}
		if (massP[y][x]->getFlag())
		{
			return;
		}
		// 爆弾マスであれば爆弾マスをすべて開ける
		if (isBombMass(y,x) && isFirst)
		{
			for (int smallY = 0; smallY < cOneSideNum; ++smallY)
			{
				for (int smallX = 0; smallX < cOneSideNum; ++smallX)
				{
					if (!isBombMass(smallY,smallX)) { continue; }
					massP[smallY][smallX]->open();
				}
			}
			isClear = false;
			return;
		}
		massP[y][x]->open();
		// 周りに爆弾があるマスだったら終了
		if (massP[y][x]->getNum() != 0)
		{
			return;
		}
		else
		{
			// 周りの8つを開く
			for (int smallY = -1; smallY < 2; ++smallY)
			{
				for (int smallX = -1; smallX < 2; ++smallX)
				{
					// 画面外、またはもう開いていれば開かない
					if (y + smallY < 0 || y + smallY > cOneSideNum - 1 || x + smallX < 0 || x + smallX > cOneSideNum - 1) { continue; }
					if (massP[y + smallY][x + smallX]->getIsOpen()) { continue; }
					openMass(y + smallY, x + smallX, false);
				}
			}
		}
	}
	//-------------------------------------------------------
	// 爆弾マスか確認する
	bool isBombMass(const int y = 0, const int x = 0)
	{
		return massP[y][x]->getType() == "BombMass";
	}
	bool isBombMass(const Vector2 checkPos = {})
	{
		return massP[checkPos.y][checkPos.x]->getType() == "BombMass";
	}
	//-------------------------------------------------------
	// 選択されたマスのフラグを反転する
	void flipMassFlag(Vector2 selectPos)
	{
		bool oppositeFlag = !massP[selectPos.y][selectPos.x]->getFlag();
		massP[selectPos.y][selectPos.x]->setFlag(oppositeFlag);
	}
	//-------------------------------------------------------
	// 移動する
	void moveMass(char input)
	{
		if (input == 'H')
		{
			--selectPos.y;
			// 位置の矯正
			if (selectPos.y < 0)
			{
				selectPos.y = 0;
			}
		}
		else if (input == 'P')
		{
			++selectPos.y;
			// 位置の矯正
			if (selectPos.y > cOneSideNum - 1)
			{
				selectPos.y = cOneSideNum - 1;
			}
		}
		else if (input == 'K')
		{
			--selectPos.x;
			// 位置の矯正
			if (selectPos.x < 0)
			{
				selectPos.x = 0;
			}
		}
		else if (input == 'M')
		{
			++selectPos.x;
			// 位置の矯正
			if (selectPos.x > cOneSideNum - 1)
			{
				selectPos.x = cOneSideNum - 1;
			}
		}
	}
	//-------------------------------------------------------
	// プレイ
	void play()
	{
		while (isClear)
		{
			// 画面表示
			printBoardScreen();
			// 入力
			char input = _getch();
			if (input == 'H' || input == 'P' || input == 'K' || input == 'M')
			{
				moveMass(input);
			}
			else if (input == 'f')
			{
				openMass(selectPos, true);
			}
			else if (input == 'd')
			{
				flipMassFlag(selectPos);
			}
			system("cls");
			if (countCloseMass() == cBombNum) { break; }
		}
	}
	//-------------------------------------------------------
	// 画面表示
	void printBoardScreen()
	{
		flagCntNum = 0;
		for (int y = 0; y < cOneSideNum; ++y)
		{
			for (int x = 0; x < cOneSideNum; ++x)
			{
				// 選択中のマスは色を変える
				int dispColor = 0x0000;
				if (selectPos.y == y && selectPos.x == x)
				{
					dispColor = 0x00A0;
					changeColor(dispColor);
				}
				// 開かれていたときはマスの情報を表示
				if (massP[y][x]->getIsOpen())
				{
					if (isBombMass(y,x))
					{
						cout << "爆";
					}
					else if (massP[y][x]->getNum() == 0)
					{
						cout << "□";
					}
					else
					{
						switch (massP[y][x]->getNum())
						{
						case 1:
							dispColor |= 0x000E;
							break;
						case 2:
							dispColor |= 0x000B;
							break;
						case 3:
							dispColor |= 0x000C;
							break;
						case 4:
							dispColor |= 0x000D;
							break;
						case 5:
							dispColor |= 0x0004;
							break;
						case 6:
							dispColor |= 0x0004;
							break;
						case 7:
							dispColor |= 0x0004;
							break;
						case 8:
							dispColor |= 0x0004;
							break;
						default:
							break;
						}
						changeColor(dispColor);
						cout << " " << massP[y][x]->getNum();
					}
				}
				// 開かれていない時はフラグが立てられているときとそうでない時を表示
				else
				{
					if (massP[y][x]->getFlag())
					{
						cout << "◆";
						++flagCntNum;
					}
					else
					{
						cout << "■";
					}
				}
				changeColor(0x0007);
			}
			if (y == cOneSideNum - 4)
			{
				cout << "　　　dキー：フラグを立てる";
			}
			else if (y == cOneSideNum - 3)
			{
				cout << "　　　fキー：マスを開く";
			}
			else if (y == cOneSideNum - 1)
			{
				cout << "　　　flag：" << setw(2) << cBombNum - flagCntNum;
			}
			cout << endl;
		}
	}
	//-------------------------------------------------------
	~GameBoard()
	{
		printBoardScreen();
		// マスオブジェクトの開放
		for (int y = 0; y < cOneSideNum; ++y)
		{
			for (int x = 0; x < cOneSideNum; ++x)
			{
				delete massP[y][x];
			}
		}
		cout << endl;
		if (isClear)
		{
			cout << "　　　congratulation!!";
		}
		else
		{
			cout << "　　　gameover...";
		}
		cout << endl;
	}
};


int main()
{
	srand((unsigned)time(NULL));
	while (true)
	{
		// オープニング画面
		int selectPosY = 0;
		while (true)
		{
			const int cChoicesNum = 5;
			cout << endl << "　　Minesweeper" << endl << endl;
			for (int i = 0; i < cChoicesNum; ++i)
			{
				if (selectPosY == i)
				{
					cout << "　　→ ";
				}
				else
				{
					cout << "　　　";
				}
				switch (i)
				{
				case 0:
					cout << "9 × 9 (10個)：初級";
					break;
				case 1:
					cout << "16 × 16 (40個)：中級";
					break;
				case 2:
					cout << "20 × 20 (82個)：上級";
					break;
				case 3:
					cout << "カスタム";
					break;
				case 4:
					cout << "やめる";
					break;
				default:
					break;
				}
				cout << endl;
			}
			cout << endl << "　fキーで決定" << endl;
			char input = _getch();
			system("cls");
			if (input == 'H')
			{
				--selectPosY;
				if (selectPosY < 0)
				{
					selectPosY = cChoicesNum - 1;
				}
			}
			else if (input == 'P')
			{
				++selectPosY;
				if (selectPosY > cChoicesNum - 1)
				{
					selectPosY = 0;
				}
			}
			else if (input == 'f')
			{
				break;
			}
		}
		// ゲームボードの生成
		GameBoard* gameBoardP = nullptr;
		if (selectPosY == 0)
		{
			gameBoardP = new GameBoard(10, 9);
		}
		else if (selectPosY == 1)
		{
			gameBoardP = new GameBoard(40, 16);
		}
		else if (selectPosY == 2)
		{
			gameBoardP = new GameBoard(82, 20);
		}
		else if (selectPosY == 3)
		{
			int customOneSideNum = 0;
			int customBombNum = 0;
			system("cls");
			cout << endl << "　爆弾の数　：";
			cin >> customBombNum;
			cout << "　一辺の長さ：";
			cin >> customOneSideNum;
			if (customBombNum > customOneSideNum * customOneSideNum)
			{
				cout << endl << "爆弾の数 が多すぎます！" << endl;
				_getch();
				system("cls");
				continue;
			}
			gameBoardP = new GameBoard(customBombNum, customOneSideNum);
			system("cls");
		}
		else
		{
			break;
		}
		selectPosY = 0;
		// プレイ
		if (gameBoardP)
		{
			gameBoardP->play();
			delete gameBoardP;
			gameBoardP = nullptr;
			cout << endl << "　　スペースキーで次へ" << endl;
			while (true)
			{
				char inp = _getch();
				if (inp == ' ')
				{
					break;
				}
			}
		}
		system("cls");
		// エンディング
		while (true)
		{
			const int cChoicesNum = 2;
			cout << endl;
			for (int i = 0; i < cChoicesNum; ++i)
			{
				if (selectPosY == i)
				{
					cout << "　→ ";
				}
				else
				{
					cout << "　　";
				}
				switch (i)
				{
				case 0:
					cout << "タイトルに戻る";
					break;
				case 1:
					cout << "やめる";
					break;
				default:
					break;
				}
				cout << endl;
			}
			cout << endl << "　　fキーで決定" << endl;
			char input = _getch();
			system("cls");
			if (input == 'H')
			{
				--selectPosY;
				if (selectPosY < 0)
				{
					selectPosY = cChoicesNum - 1;
				}
			}
			else if (input == 'P')
			{
				++selectPosY;
				if (selectPosY > cChoicesNum - 1)
				{
					selectPosY = 0;
				}
			}
			else if (input == 'f')
			{
				break;
			}
		}
		if (selectPosY == 0)
		{
			continue;
		}
		else
		{
			break;
		}
	}
    return 0;
}

