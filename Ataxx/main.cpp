#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <iostream>
using namespace std;
#define KEY_DOWN  80
#define KEY_UP    72
#define KEY_LEFT  75
#define KEY_RIGHT 77
#define KEY_ESC   27

// ������Ƶ�(x, y)����xΪ��ֱ����yΪˮƽ����
void gotoxy(int x, int y)
{
	COORD coord = { y, x };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);//���ƹ�� 
}

// ��ӡ����
void print_board() {
	cout << "     1   2   3   4   5   6   7   8   " << endl;
	cout << "  �����ө��ө��ө��ө��ө��ө��ө���" << endl;
	for (int i = 1; i <= 7; i++)
	{
		cout << ' ' << char('A' + i - 1) << "��  ��  ��  ��  ��  ��  ��  ��  ��" << endl;
		cout << "  �ġ��ࡪ�ࡪ�ࡪ�ࡪ�ࡪ�ࡪ�ࡪ��" << endl;
	}
	cout << " H��  ��  ��  ��  ��  ��  ��  ��  ��" << endl;;
	cout << "  �����۩��۩��۩��۩��۩��۩��۩���" << endl;
}

int main()
{
	print_board();
	int x = 0, y = 0;
	gotoxy(2 * (x + 1), 4 + y * 4);
	while(true) {
		int c = _getch();//����Ҫ���س����Ͷ��� 
		// ��ȡ���������
		if (!isascii(c)) {
			c = _getch();
			switch (c) {
			case KEY_DOWN: 
				if (x < 7) {
					x++;
					gotoxy(2 * (x + 1), 4 + y * 4);
				}
				break;
			case KEY_UP: 
				if (x > 0) {
					x--; 
					gotoxy(2 * (x + 1), 4 + y * 4);
				}
				break;
			case KEY_LEFT: 
				if (y > 0) {
					y--; 
					gotoxy(2 * (x + 1), 4 + y * 4);
				}
				break;
			case KEY_RIGHT:
				if (y < 7) {
					y++;
					gotoxy(2 * (x + 1), 4 + y * 4);
				}
				break;
			default: break;
			}
		}
		// �����ַ�����
		else {
			if (c == ' ') {
				cout << "��";
				gotoxy(2 * (x + 1), 4 * (y + 1));
			}
			else if (c == KEY_ESC) {
				gotoxy(18, 0);
				break;
			}
			else if (c == 'c') {
				system("cls");   // ����
				print_board();
				x = 0, y = 0;
				gotoxy(2 * (x + 1), 4 * (y + 1));
			}
		}
	}
	return 0;
}
