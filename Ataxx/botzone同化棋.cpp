#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include "jsoncpp/json.h" // C++编译时默认包含此库

using namespace std;

int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 2, whitePieceCount = 2;

int depth0=4; 
static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
{ 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
{ -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };
int beginPos[5][1000][2]={0}, possiblePos[5][1000][2]={0}, posCount[5] = {0}, dir;
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
void unmakemove(int x0, int y0, int x1, int y1, int color,int effectivePoints[8][2],int currCount)
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
	for (int i=0;i<currCount;i++) // 影响邻近8个位置
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
bool findpossblePos(int depth,int color)
{	
	posCount[depth]= 0;
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
	if(posCount[depth]==0) return 0;
	return 1;
}
int minq(int depth,int alpha,int beta); 
int maxq(int depth,int alpha,int beta)
{
	if(depth>=depth0) return (blackPieceCount-whitePieceCount)*currBotColor;
	bool x=findpossblePos(depth,currBotColor);int choice=-1;
	if(!x) return minq(depth+1,alpha,beta);
	for(int i=0;i<posCount[depth];i++)
	{
	    int effectivePoints[8][2]={0};int currCount=0;
		ProcStep(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], currBotColor,effectivePoints,currCount);
		int value=minq(depth+1,alpha,beta);
		
		unmakemove(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], currBotColor,effectivePoints,currCount);	
		if(value>alpha) 
		{
			alpha=value;choice=i;
		}
		if(alpha>=beta) return alpha;
	}
	if(depth==0) return choice;
	else return alpha;
}
int minq(int depth,int alpha,int beta)
{
	if(depth>=depth0) return (blackPieceCount-whitePieceCount)*currBotColor;
	bool x=findpossblePos(depth,-currBotColor);
	if(!x) return maxq(depth+1,alpha,beta);
	for(int i=0;i<posCount[depth];i++)
	{
	    int effectivePoints[8][2]={0};int currCount=0;
		ProcStep(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], -currBotColor,effectivePoints,currCount);	
		int value=maxq(depth+1,alpha,beta);
		unmakemove(beginPos[depth][i][0], beginPos[depth][i][1], possiblePos[depth][i][0], possiblePos[depth][i][1], -currBotColor,effectivePoints,currCount);	
	    if(value<beta)
	    beta=value;
	    if(alpha>=beta)  return beta;
	}
	return beta;
}
int main()
{
	int x0, y0, x1, y1;

	// 初始化棋盘
	gridInfo[0][0] = gridInfo[6][6] = 1;  //|黑|白|
	gridInfo[6][0] = gridInfo[0][6] = -1; //|白|黑|

	// 读入JSON
	string str;
	getline(cin, str);
	Json::Reader reader;
	Json::Value input;
	reader.parse(str, input);
    int a[8][2]={0},t=0;
	// 分析自己收到的输入和自己过往的输出，并恢复状态
	int turnID = input["responses"].size();
	currBotColor = input["requests"][(Json::Value::UInt) 0]["x0"].asInt() < 0 ? 1 : -1; // 第一回合收到坐标是-1, -1，说明我是黑方
	for (int i = 0; i < turnID; i++)
	{
		t=0;
		// 根据这些输入输出逐渐恢复状态到当前回合
		x0 = input["requests"][i]["x0"].asInt();
		y0 = input["requests"][i]["y0"].asInt();
		x1 = input["requests"][i]["x1"].asInt();
		y1 = input["requests"][i]["y1"].asInt();
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, -currBotColor,a,t); // 模拟对方落子
		t=0;
		x0 = input["responses"][i]["x0"].asInt();
		y0 = input["responses"][i]["y0"].asInt();
		x1 = input["responses"][i]["x1"].asInt();
		y1 = input["responses"][i]["y1"].asInt();
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, currBotColor,a,t); // 模拟己方落子
	}
	// 看看自己本回合输入
	t=0;
	x0 = input["requests"][turnID]["x0"].asInt();
	y0 = input["requests"][turnID]["y0"].asInt();
	x1 = input["requests"][turnID]["x1"].asInt();
	y1 = input["requests"][turnID]["y1"].asInt();
	if (x1 >= 0)
		ProcStep(x0, y0, x1, y1, -currBotColor,a,t); // 模拟对方落子

	// 找出合法落子点

    if((blackPieceCount+whitePieceCount)>=36) depth0=5;
	// 做出决策（你只需修改以下部分）

	int startX, startY, resultX, resultY, bestchoice = 0;
	bestchoice=maxq(0,-50,50);
	if (bestchoice>=0)
	{
		startX = beginPos[0][bestchoice][0];
		startY = beginPos[0][bestchoice][1];
		resultX = possiblePos[0][bestchoice][0];
		resultY = possiblePos[0][bestchoice][1];
	}
	else
	{
		startX = -1;
		startY = -1;
		resultX = -1;
		resultY = -1;
	}

	// 决策结束，输出结果（你只需修改以上部分）

	Json::Value ret;
	ret["response"]["x0"] = startX;
	ret["response"]["y0"] = startY;
	ret["response"]["x1"] = resultX;
	ret["response"]["y1"] = resultY;
	Json::FastWriter writer;
	cout << writer.write(ret) << endl;
	return 0;
}

