#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <fstream>
using namespace std;

int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 2, whitePieceCount = 2;
int blackPieceCount0 = 2, whitePieceCount0 = 2;
const int depth0 = 4;
static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
{ 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
{ -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };
int beginPos[7][1000][2] = { 0 }, possiblePos[7][1000][2] = { 0 }, posCount[7] = { 0 }, dir;

// 判断是否在地图内
inline bool inMap(int x, int y)
{
	if (x < 0 || x > 6 || y < 0 || y > 6)
		return false;
	return true;
}

// 向Direction方向改动坐标，并返回是否越界
inline bool MoveStep(int &x, int &y, int Direction)
{
	x = x + delta[Direction][0];
	y = y + delta[Direction][1];
	return inMap(x, y);
}
// 将光标移到(x, y)处，x为垂直方向，y为水平方向
void gotoxy(int x, int y)
{
	COORD coord = { y, x };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);//控制光标 
}
// 在坐标处落子，检查是否合法或模拟落子
bool ProcStep(int x0, int y0, int x1, int y1, int color,int effectivePoints[8][2],int &currCount)
{
	if (color == 0)
		return false;
	if (x1 == -1) // 无路可走，跳过此回合
		return true;
	if (!inMap(x0, y0) || !inMap(x1, y1)) // 超出边界
		return false;
	if (gridInfo[x0][y0] != color)
		return false;
	int dx, dy, x, y, dir;
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // 保证不会移动到原来位置，而且移动始终在5×5区域内
		return false;
	if (gridInfo[x1][y1] != 0) // 保证移动到的位置为空
		return false;
	if (dx == 2 || dy == 2) // 如果走的是5×5的外围，则不是复制粘贴
		gridInfo[x0][y0] = 0;
	else
	{
		if (color == 1)
			blackPieceCount++;
		else
			whitePieceCount++;
	}
	gridInfo[x1][y1] = color;
	currCount=0; 
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		x = x1 + delta[dir][0];
		y = y1 + delta[dir][1];
		if (!inMap(x, y))
			continue;
		if (gridInfo[x][y] == -color)
		{
			effectivePoints[currCount][0] = x;
			effectivePoints[currCount][1] = y;
			currCount++;
			gridInfo[x][y] = color;
		}
	}
	if (currCount != 0)
	{
		if (color == 1)
		{
			blackPieceCount += currCount;
			whitePieceCount -= currCount;
		}
		else
		{
			whitePieceCount += currCount;
			blackPieceCount -= currCount;
		}
	}
	return true;
}
void unmakemove(int x0, int y0, int x1, int y1, int color,int effectivePoints[8][2],int currCount) //棋子复位 
{
	int dx, dy;
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	gridInfo[x1][y1] = 0;
	if (dx == 2 || dy == 2) // 如果走的是5×5的外围，则不是复制粘贴
	gridInfo[x0][y0] = color;
	else
	{
		if (color == 1)
			blackPieceCount--;
		else
			whitePieceCount--;
	}
	for (int i = 0; i < currCount; i++) // 影响邻近8个位置
	gridInfo[effectivePoints[i][0]][effectivePoints[i][1]]=-color;
	if (currCount != 0)
	{
		if (color == 1)
		{
			blackPieceCount -= currCount;
			whitePieceCount += currCount;
		}
		else
		{
			whitePieceCount -= currCount;
			blackPieceCount += currCount;
		}
	}
}
bool findpossblePos(int depth,int color)//找到可行步骤 
{	
	posCount[depth] = 0;
	for (int y0 = 0; y0 < 7; y0++)
		for (int x0 = 0; x0 < 7; x0++)
		{
			if (gridInfo[x0][y0] != color)
				continue;
			for (dir = 0; dir < 24; dir++)
			{
				int x1 = x0 + delta[dir][0];
				int y1 = y0 + delta[dir][1];
				if (!inMap(x1, y1))
					continue;
				if (gridInfo[x1][y1] != 0)
					continue;
				beginPos[depth][posCount[depth]][0] = x0;
				beginPos[depth][posCount[depth]][1] = y0;
				possiblePos[depth][posCount[depth]][0] = x1;
				possiblePos[depth][posCount[depth]][1] = y1;
				posCount[depth]++;
			}
		}
	if(posCount[depth] == 0) return 0;
	return 1;
}
//minimax搜索+alpha-beta剪枝 
int minq(int depth,int alpha,int beta); 
int maxq(int depth,int alpha,int beta)
{
	if(depth >= depth0) return (blackPieceCount - whitePieceCount) * currBotColor;
	bool x = findpossblePos(depth,currBotColor); int choice = -1;
	if(!x) return minq(depth+1,alpha,beta);
	for(int i = 0; i < posCount[depth]; i++)
	{
	    int effectivePoints[8][2] = { 0 }; int currCount = 0;
		ProcStep(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], currBotColor,effectivePoints,currCount);
		int value = minq(depth+1,alpha,beta);
		unmakemove(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], currBotColor,effectivePoints,currCount);	
		if(value > alpha) 
		{
			alpha = value; choice = i;
		}
		if(alpha >= beta) return alpha;
	}
	if(depth == 0) return choice;
	else return alpha;
}
int minq(int depth,int alpha,int beta)
{
	if(depth >= depth0) return (blackPieceCount - whitePieceCount) * currBotColor;
	bool x = findpossblePos(depth,-currBotColor);
	if(!x) return maxq(depth+1,alpha,beta);
	for(int i = 0; i < posCount[depth]; i++)
	{
	    int effectivePoints[8][2] = { 0 };int currCount = 0;
		ProcStep(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], -currBotColor,effectivePoints,currCount);	
		int value = maxq(depth+1,alpha,beta);
		unmakemove(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], -currBotColor,effectivePoints,currCount);	
	    if(value < beta)
	    beta = value;
	    if(alpha >= beta)  return beta;
	}
	return beta;
}

void print_board() // 打印棋盘
{
	cout << "     0   1   2   3   4   5   6   " << endl;
	cout << "  ┏ ━ ┯ ━ ┯ ━ ┯ ━ ┯ ━ ┯ ━ ┯ ━ ┓" << endl;
	for (int i = 1; i <= 6; i++)
	{
		cout << ' ' << i-1 << "┃   │   │   │   │   │   │   ┃" << endl;
		cout << "  ┠ —┼ —┼ —┼ —┼ —┼ —┼ —┨" << endl;
	}
	cout << " 6┃   │   │   │   │   │   │   ┃" << endl;;
	cout << "  ┗ ━ ┷ ━ ┷ ━ ┷ ━ ┷ ━ ┷ ━ ┷ ━ ┛" << endl;
}
void print() //打印 
{
	system("cls");
	print_board(); 
	for(int i = 0; i < 7 ; i++)
	{
		for(int j = 0; j < 7; j++)
		{
			if(gridInfo[i][j] == -1)
			{
				gotoxy(2 * (i + 1), 4 + j * 4);
				cout << "●";
			}
			else if(gridInfo[i][j] == 1)
			{
				gotoxy(2 * (i + 1), 4 + j * 4);
				cout << "○";
			}
		}
	}
	cout << endl << endl << "blackPieceCount:" << blackPieceCount << "   " << "whitePieceCount:" << whitePieceCount << endl;
	if(currBotColor == 1) cout << "你控制白棋  输入-1 -1 -1 -1返回菜单栏" << endl;
	if(currBotColor == -1) cout << "你控制黑棋  输入-1 -1 -1 -1返回菜单栏" << endl; 
	
}
void gamecom() //电脑输入 
{
	bool t1;
	t1 = findpossblePos(0,currBotColor);
	if(t1 == 0) return;
	blackPieceCount0 = blackPieceCount, whitePieceCount0 = whitePieceCount;
	int choice = maxq(0,-50,50);
	int c = 0,a[8][2] = { 0 };
	blackPieceCount = blackPieceCount0, whitePieceCount = whitePieceCount0;//保护黑白子数据 
    ProcStep(beginPos[0][choice][0], beginPos[0][choice][1], possiblePos[0][choice][0], possiblePos[0][choice][1],currBotColor,a,c);
    print();
}
bool gameplayer() //玩家输入 
{
	bool t=false,t2;
	int x0,x1,y0,y1,c = 0,a[8][2] = { 0 };
    t2 = findpossblePos(0,-currBotColor);
    if(t2 == 0) return 0;
    while(t == false)
    {
        cout << "Please enter the coordinates 请输入坐标：（输入格式：初始坐标x 初始坐标y 到达位置x1 到达位置y1）" << endl;
		cin >> x0 >> y0 >> x1 >> y1;c = 0;if(x0 == -1) return 0;
        t = ProcStep(x0, y0, x1, y1, -currBotColor, a, c);
        if(t == 0){
	    print(); 
		cout << "Input error! 输入有误！" << endl;} 
    }
    print();
    return 1;
} 
bool primaryinput() // 首次输入 
{
	int x0,x1,y0,y1;int a[8][2] = { 0 },c = 0;bool t1,t2,t = false;
	do
	{
      	cout << "请选择先手或后手（输入1为先手，-1为后手）：";
		int m;cin >> m;
		if(m == 1)
		{
		    print();
			cout << "Please enter the coordinates 请输入坐标：（输入格式：初始坐标x 初始坐标y 到达位置x1 到达位置y1）" << endl;
	        cin >> x0 >> y0 >> x1 >> y1 ;if(x0 < 0) return 0;
	        t = ProcStep(x0, y0, x1, y1, 1, a, c)||ProcStep(x0, y0, x1, y1, -1, a, c);
	        if(t == 0){
	        print(); 
		    cout << "Input error! 输入有误！" << endl;} 
		    else{
		        if(x0 == -1) currBotColor = 1;
	            else{
	        	    if(x0+y0 == 6) currBotColor = 1;
	        	    else currBotColor = -1;
	                }
	        }
	    }
		else if(m == -1) {t = 1;currBotColor = 1;} 
	    else {print();cout << "Input error! 输入有误！" << endl;}
	}
	while(t == false);
	print();
	return 1;
}
void gameon() // 控制游戏进程 
{
	bool t1,t2;
	while(blackPieceCount + whitePieceCount < 49)
	{
		gamecom(); bool m = gameplayer();
		if(m == 0) break;
		t1 = findpossblePos(0,currBotColor);t2 = findpossblePos(0,-currBotColor);
		if(!t1 || !t2) break;
    }
    print(); 
    if((t1 == 0 && currBotColor == -1)||(t2 == 0 && currBotColor == 1)) blackPieceCount = 49-whitePieceCount;
    else if((t1 == 0 && currBotColor == 1)||(t2 == 0 && currBotColor == -1)) whitePieceCount = 49-blackPieceCount;//填充残局 
	if((blackPieceCount - whitePieceCount) * currBotColor < 0) cout << "YOU WIN!";
	else if((blackPieceCount - whitePieceCount) * currBotColor > 0) cout << "YOU LOSE!"; 
	cin.get();  cin.get();
}
void play() //新开始 
{
	for(int i = 0; i < 7; i++)
	for(int j = 0; j < 7; j++)
	gridInfo[i][j] = 0;
	blackPieceCount = 2;whitePieceCount = 2;currBotColor = 0; 
	gridInfo[0][0] = 1;gridInfo[6][6] = 1;gridInfo[6][0] = -1;gridInfo[0][6] = -1;
	print();
	bool t = primaryinput();if(t == 0) return;
	gameon();
}
void append() // 存盘 
{
	ofstream FILE;
	FILE.open("data.txt",ios::out);
    if(!FILE)
	{  
		cout << "Can't open the file" << endl;
		exit(1);
    }
    for(int i = 0; i<7; i++)
    {
		for(int j = 0; j<7; j++)
        {
        	FILE << gridInfo[i][j] << " ";
		}
		
    }
    FILE << currBotColor;
}
void replay() // 复盘 
{
	ifstream FILE;
	FILE.open("data.txt");
    if(!FILE)
	{  
		cout << "Can't open the file" << endl;
		exit(1);
    }
    blackPieceCount = 0; whitePieceCount = 0;
    for(int i = 0; i < 7; i++)
    for(int j = 0; j < 7; j++)
    {
    	FILE >> gridInfo[i][j];
    	{
    		if(gridInfo[i][j] == 1) blackPieceCount++;
    		if(gridInfo[i][j] == -1) whitePieceCount++;
		}
	}
	FILE >> currBotColor;
	print();
	bool m = gameplayer();
	if(m == 0) return;
	gameon();
}
bool startgame() // 开始游戏 
{
    system("cls");
    cout << "┏ ━ ━ ━ ━ ━ ━ ━ ━┓" << endl;
	cout << "┃ 欢迎来玩同化棋 ┃" << endl;
    cout << "┃ 1.新开局       ┃" << endl;
    cout << "┃ 2.存盘         ┃" << endl;
    cout << "┃ 3.复盘         ┃" << endl;
    cout << "┃ 4.退出         ┃" << endl;
	cout << "┗ ━ ━ ━ ━ ━ ━ ━ ━┛" << endl; 
    int n;
    cin >> n;
    switch(n)
    {
    	case 1: play(); break;
    	case 2: append(); break;
    	case 3: replay(); break;
    	case 4: return 0;
	}
	return 1;
}
int main()
{
    bool m = true;
    while(m)	m = startgame();
	return 0;
}
