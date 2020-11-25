#include <iostream>
#include <set>
#include <string>
#include <cassert>
#include <cstring> // 注意memset是cstring里的
#include <algorithm>
//#include "jsoncpp/json.h"
using std::vector;
using std::sort;
using std::unique;
using std::set;
using std::string;

constexpr int PLAYER_COUNT = 3;

enum class CardComboType
{
	PASS, // 过
	SINGLE, // 单张
	PAIR, // 对子
	STRAIGHT, // 顺子
	STRAIGHT2, // 双顺
	TRIPLET, // 三条
	TRIPLET1, // 三带一
	TRIPLET2, // 三带二
	BOMB, // 炸弹
	QUADRUPLE2, // 四带二（只）
	QUADRUPLE4, // 四带二（对）
	PLANE, // 飞机
	PLANE1, // 飞机带小翼
	PLANE2, // 飞机带大翼
	SSHUTTLE, // 航天飞机
	SSHUTTLE2, // 航天飞机带小翼
	SSHUTTLE4, // 航天飞机带大翼
	ROCKET, // 火箭
	INVALID // 非法牌型
};

int cardComboScores[] = {
	0, // 过
	1, // 单张
	2, // 对子
	6, // 顺子
	6, // 双顺
	4, // 三条
	4, // 三带一
	4, // 三带二
	10, // 炸弹
	8, // 四带二（只）
	8, // 四带二（对）
	8, // 飞机
	8, // 飞机带小翼
	8, // 飞机带大翼
	10, // 航天飞机（需要特判：二连为10分，多连为20分）
	10, // 航天飞机带小翼
	10, // 航天飞机带大翼
	16, // 火箭
	0 // 非法牌型
};

#ifndef _BOTZONE_ONLINE
string cardComboStrings[] = {
	"PASS",
	"SINGLE",
	"PAIR",
	"STRAIGHT",
	"STRAIGHT2",
	"TRIPLET",
	"TRIPLET1",
	"TRIPLET2",
	"BOMB",
	"QUADRUPLE2",
	"QUADRUPLE4",
	"PLANE",
	"PLANE1",
	"PLANE2",
	"SSHUTTLE",
	"SSHUTTLE2",
	"SSHUTTLE4",
	"ROCKET",
	"INVALID"
};
#endif

// 用0~53这54个整数表示唯一的一张牌
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// 除了用0~53这54个整数表示唯一的牌，
// 这里还用另一种序号表示牌的大小（不管花色），以便比较，称作等级（Level）
// 对应关系如下：
// 3 4 5 6 7 8 9 10	J Q K	A	2	小王	大王
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/**
 * 将Card变成Level
 */
constexpr Level card2level(Card card)
{
	return card / 4 + card / 53;
}

short used_card[MAX_LEVEL + 1] = {0};

//从当前开始是哥哥我写的 
int find_Max_Seq(short count[MAX_LEVEL + 1], int minlevel, int num){ //检查牌组最多能递增几个 
	int len = 0;
	for (unsigned c = minlevel; c <= MAX_STRAIGHT_LEVEL; c++){
		if (count[c] >= num && count[c] != 4){
			len++;
		}
		else break;
	}		
	return len;
}

short find_all_deck_maxlevel(){
	Level i = MAX_STRAIGHT_LEVEL;
	for(; i >= 0; i--){
		if(used_card[i] != 4) return i;
	}
	return i;
}
//从当前结束是哥哥我写的 
struct CardCombo
{
	// 表示同等级的牌有多少张
	// 会按个数从大到小、等级从大到小排序
	struct CardPack
	{
		Level level;
		short count;

		bool operator< (const CardPack& b) const
		{
			if (count == b.count)
				return level > b.level;
			return count > b.count;
		}
	};
	vector<Card> cards; // 原始的牌，未排序
	vector<CardPack> packs; // 按数目和大小排序的牌种
	CardComboType comboType; // 算出的牌型
	Level comboLevel = 0; // 算出的大小序

	/**
	 * 检查个数最多的CardPack递减了几个
	 */
	int findMaxSeq() const
	{
		for (unsigned c = 1; c < packs.size(); c++)
			if (packs[c].count != packs[0].count ||
				packs[c].level != packs[c - 1].level - 1)
				return c;
		return packs.size();
	}

	/**
	* 这个牌型最后算总分的时候的权重
	*/
	int getWeight() const
	{
		if (comboType == CardComboType::SSHUTTLE ||
			comboType == CardComboType::SSHUTTLE2 ||
			comboType == CardComboType::SSHUTTLE4)
			return cardComboScores[(int)comboType] + (findMaxSeq() > 2) * 10;
		return cardComboScores[(int)comboType];
	}

	// 创建一个空牌组
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	 * 通过Card（即short）类型的迭代器创建一个牌型
	 * 并计算出牌型和大小序等
	 * 假设输入没有重复数字（即重复的Card）
	 */
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// 特判：空
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// 每种牌有多少个
		short counts[MAX_LEVEL + 1] = {};

		// 同种牌的张数（有多少个单张、对子、三条、四条）
		short countOfCount[5] = {};

		cards = vector<Card>(begin, end);
		for (Card c : cards)
			counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
			{
				packs.push_back(CardPack{ l, counts[l] });
				countOfCount[counts[l]]++;
			}
		sort(packs.begin(), packs.end());

		// 用最多的那种牌总是可以比较大小的
		comboLevel = packs[0].level;

		// 计算牌型
		// 按照 同种牌的张数 有几种 进行分类
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // 只有一类牌
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// 只有若干单张
				if (curr == 1)
				{
					comboType = CardComboType::SINGLE;
					return;
				}
				if (curr == 2 && packs[1].level == level_joker)
				{
					comboType = CardComboType::ROCKET;
					return;
				}
				if (curr >= 5 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT;
					return;
				}
				break;
			case 2:
				// 只有若干对子
				if (curr == 1)
				{
					comboType = CardComboType::PAIR;
					return;
				}
				if (curr >= 3 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT2;
					return;
				}
				break;
			case 3:
				// 只有若干三条
				if (curr == 1)
				{
					comboType = CardComboType::TRIPLET;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::PLANE;
					return;
				}
				break;
			case 4:
				// 只有若干四条
				if (curr == 1)
				{
					comboType = CardComboType::BOMB;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::SSHUTTLE;
					return;
				}
			}
			break;
		case 2: // 有两类牌
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// 三条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 三带一
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET1;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE1;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// 三带二
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE2;
						return;
					}
				}
			}
			if (kindOfCountOfCount[1] == 4)
			{
				// 四条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 四条带两只 * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE2;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// 四条带两对 * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE4;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE4;
						return;
					}
				}
			}
		}

		comboType = CardComboType::INVALID;
	}

	/**
	 * 判断指定牌组能否大过当前牌组（这个函数不考虑过牌的情况！）
	 */
	bool canBeBeatenBy(const CardCombo& b) const
	{
		if (comboType == CardComboType::INVALID || b.comboType == CardComboType::INVALID)
			return false;
		if (b.comboType == CardComboType::ROCKET)
			return true;
		if (b.comboType == CardComboType::BOMB)
			switch (comboType)
			{
			case CardComboType::ROCKET:
				return false;
			case CardComboType::BOMB:
				return b.comboLevel > comboLevel;
			default:
				return true;
			}
		return b.comboType == comboType && b.cards.size() == cards.size() && b.comboLevel > comboLevel;
	}

	/**
	 * 从指定手牌中寻找第一个能大过当前牌组的牌组
	 * 如果随便出的话只出第一张
	 * 如果不存在则返回一个PASS的牌组
	 */
	 
	 
	//从当前开始是哥哥我写的 
template <typename CARD_ITERATOR>
    CardCombo find_min_comb(CARD_ITERATOR begin, CARD_ITERATOR end) const// 找到最小牌组合  返回组合 
    {
		// 特判：空
		if (begin == end)
		{
			return CardCombo();
		}
        
        short all_deck_maxlevel = find_all_deck_maxlevel();
        
		// 每种牌有多少个
		short COUNTS[MAX_LEVEL + 1] = {};

		vector<Card> CARDS(begin, end);
		vector<Card> usecard;//要出的卡组 
		sort(CARDS.begin(), CARDS.end());
		for (Card c : CARDS)
			COUNTS[card2level(c)]++;
	
		int minlevel = card2level(CARDS[0]);
		int len1 = find_Max_Seq(COUNTS, minlevel, 1);// STRAIGHT的长度
		int len2 = find_Max_Seq(COUNTS, minlevel, 2);// STRAIGHT2的长度
		int len3 = find_Max_Seq(COUNTS, minlevel, 3);// plane的长度 
		CARD_ITERATOR second = begin;
		if(COUNTS[minlevel] == 4){//最小牌组合是炸弹 不使用比较好吧 
			second ++; second ++; second ++; second ++;
			CardCombo new_CARDS = find_min_comb(second, end);//往后找组合 
			if(new_CARDS.comboType == CardComboType::PASS) {
				return CardCombo(begin, end);          //只有炸弹组合 
			} 
			else {
				return new_CARDS; //或者返回之后的组合 
			}
		}
    	if(len3 >= 2){//plane 可以形成 
			int levelx = minlevel;//当前level 
    		int t = 0;//计数，使得有三张牌 
    		unsigned i = 0; int l = len3;
    		for(; i < CARDS.size() && len3 > 0; i++){
    			if(t == 3){
    				t = 0;
    				levelx ++;
    				len3--;
    				if(len3 == 0) {
    					i++;break;
					}
				}
    			if(card2level(CARDS[i]) == levelx){
    				usecard.push_back(CARDS[i]);
    				t++;
				}
			}
			vector<Card> single_card; levelx = -1; 
			for(;i < CARDS.size() && l > 0 && card2level(CARDS[i]) < all_deck_maxlevel; i++){
    			if(card2level(CARDS[i]) != levelx && COUNTS[card2level(CARDS[i])] != 4){
    				single_card.push_back(CARDS[i]);
    				levelx = card2level(CARDS[i]);
    				l--;
				}
				if(l == 0) {
    				usecard.insert(usecard.end(), single_card.begin(), single_card.end());
					break;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		} 
		else if(len2 >= 3){ //STRAIGHT2可以形成 
    		int levelx = minlevel;//当前level 
    		int t = 0;//计数，使得有一对牌 
    		for(unsigned i = 0; i < CARDS.size() && len2 > 0; i++){
    			if(t == 2){
    				t = 0;
    				levelx ++;
    				len2--;
    				if(len2 == 0) break;
				}
    			if(card2level(CARDS[i]) == levelx){
    				usecard.push_back(CARDS[i]);
    				t++;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
    	else if(len1 >= 5){ //STRAIGHT可以形成
    		int levelx = minlevel;//当前level 
    		int t = 0;
    		for(unsigned i = 0; i < CARDS.size() && len1 > 0; i++){
    			if(t == 1){ //只需要1张牌 
    				t = 0;
    				levelx ++;
    				len1 --;
    				if(len1 == 0) break;
				}
    			if(card2level(CARDS[i]) == levelx){
    				usecard.push_back(CARDS[i]);
    				t++;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
		else if(COUNTS[minlevel] == 3){
			unsigned i = 0;
			for(; i < 3; i++){
				usecard.push_back(CARDS[i]);
			}
			unsigned j = i;
			for(; i < CARDS.size(); i++){
				if(COUNTS[card2level(CARDS[i])] == 1 && card2level(CARDS[i]) < all_deck_maxlevel){
					usecard.push_back(CARDS[i]);
					return CardCombo(usecard.begin(), usecard.end());
				}
			}
			for(; j < CARDS.size(); j++){
				if(COUNTS[card2level(CARDS[j])] == 2 && card2level(CARDS[j]) < all_deck_maxlevel && COUNTS[card2level(CARDS[j])] != 4){
					usecard.push_back(CARDS[j]);usecard.push_back(CARDS[j + 1]);
					return CardCombo(usecard.begin(), usecard.end());
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
		else if(COUNTS[minlevel] == 2){
			unsigned i = 0;
			usecard.push_back(CARDS[i]);i++; 
			usecard.push_back(CARDS[i]);i++;
			int t;
			for(; i < CARDS.size(); i++){
				if(COUNTS[card2level(CARDS[i])] == 3 && card2level(CARDS[i]) < all_deck_maxlevel && COUNTS[card2level(CARDS[i])] != 4){
					usecard.push_back(CARDS[i]);
					t++;
					if(t == 3) break;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
		else if(COUNTS[minlevel] == 1){
			unsigned i = 0;
			usecard.push_back(CARDS[i]);i++; int t;
			for(; i < CARDS.size(); i++){
				if(COUNTS[card2level(CARDS[i])] == 3 && card2level(CARDS[i]) < all_deck_maxlevel && COUNTS[card2level(CARDS[i])] != 4){
					usecard.push_back(CARDS[i]);
					t++;
					if(t == 3) break;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		} 
	}
	//从当前结束是哥哥我写的
	
	
	
	
	template <typename CARD_ITERATOR>
	CardCombo findFirstValid(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		if (comboType == CardComboType::PASS) // 如果不需要大过谁，只需要随便出
		{
			/*
			CARD_ITERATOR second = begin;
			second++;
			return CardCombo(begin, second); // 那么就出第一张牌……*/
			return find_min_comb(begin, end);//哥哥自己的 
		}

		// 然后先看一下是不是火箭，是的话就过
		if (comboType == CardComboType::ROCKET)
			return CardCombo();

		// 现在打算从手牌中凑出同牌型的牌
		auto deck = vector<Card>(begin, end); // 手牌
		short counts[MAX_LEVEL + 1] = {};

		unsigned short kindCount = 0;

		// 先数一下手牌里每种牌有多少个
		for (Card c : deck)
			counts[card2level(c)]++;

		// 手牌如果不够用，直接不用凑了，看看能不能炸吧
		if (deck.size() < cards.size())
			goto failure;

		// 再数一下手牌里有多少种牌
		for (short c : counts)
			if (c)
				kindCount++;

		// 否则不断增大当前牌组的主牌，看看能不能找到匹配的牌组
		{
			// 开始增大主牌
			int mainPackCount = findMaxSeq();
			bool isSequential =
				comboType == CardComboType::STRAIGHT ||
				comboType == CardComboType::STRAIGHT2 ||
				comboType == CardComboType::PLANE ||
				comboType == CardComboType::PLANE1 ||
				comboType == CardComboType::PLANE2 ||
				comboType == CardComboType::SSHUTTLE ||
				comboType == CardComboType::SSHUTTLE2 ||
				comboType == CardComboType::SSHUTTLE4;
			for (Level i = 1; ; i++) // 增大多少
			{
				for (int j = 0; j < mainPackCount; j++)
				{
					int level = packs[j].level + i;

					// 各种连续牌型的主牌不能到2，非连续牌型的主牌不能到小王，单张的主牌不能超过大王
					if ((comboType == CardComboType::SINGLE && level > MAX_LEVEL) ||
						(isSequential && level > MAX_STRAIGHT_LEVEL) ||
						(comboType != CardComboType::SINGLE && !isSequential && level >= level_joker))
						goto failure;

					// 如果手牌中这种牌不够，就不用继续增了
					if (counts[level] < packs[j].count)
						goto next;
				}

				{
					// 找到了合适的主牌，那么从牌呢？
					// 如果手牌的种类数不够，那从牌的种类数就不够，也不行
					if (kindCount < packs.size())
						continue;

					// 好终于可以了
					// 计算每种牌的要求数目吧
					short requiredCounts[MAX_LEVEL + 1] = {};
					for (int j = 0; j < mainPackCount; j++)
						requiredCounts[packs[j].level + i] = packs[j].count;
					for (unsigned j = mainPackCount; j < packs.size(); j++)
					{
						Level k;
						for (k = 0; k <= MAX_LEVEL; k++)
						{
							if (requiredCounts[k] || counts[k] < packs[j].count)
								continue;
							requiredCounts[k] = packs[j].count;
							break;
						}
						if (k == MAX_LEVEL + 1) // 如果是都不符合要求……就不行了
							goto next;
					}


					// 开始产生解
					vector<Card> solve;
					for (Card c : deck)
					{
						Level level = card2level(c);
						if (requiredCounts[level])
						{
							solve.push_back(c);
							requiredCounts[level]--;
						}
					}
					return CardCombo(solve.begin(), solve.end());
				}

			next:
				; // 再增大
			}
		}

	failure:
		// 实在找不到啊
		// 最后看一下能不能炸吧

		for (Level i = 0; i < level_joker; i++)
			if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // 如果对方是炸弹，能炸的过才行
			{
				// 还真可以啊……
				Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
				return CardCombo(bomb, bomb + 4);
			}

		// 有没有火箭？
		if (counts[level_joker] + counts[level_JOKER] == 2)
		{
			Card rocket[] = { card_joker, card_JOKER };
			return CardCombo(rocket, rocket + 2);
		}

		// ……
		return CardCombo();
	}

	void debugPrint()
	{
#ifndef _BOTZONE_ONLINE
		std::cout << "【" << cardComboStrings[(int)comboType] <<
			"共" << cards.size() << "张，大小序" << comboLevel << "】";
#endif
	}
};
set<Card> myCards;
CardCombo lastValidCombo;
int main()
{
	Card a[20];
	int n;int m;
	Card b[20];
	std::cin >> n;
	for(int i = 0; i < n; i++){
		std::cin >> a[i];
	}
	myCards.insert(a, a + n);
	lastValidCombo = CardCombo();
	CardCombo myAction = lastValidCombo.findFirstValid(myCards.begin(), myCards.end());
	for(auto i = myAction.cards.begin(); i !=  myAction.cards.end(); i++){
		std::cout << *i << " ";
	}
	std::cout << std::endl;
	
	
	return 0;
}

