#include <iostream>
#include <set>
#include <string>
#include <cassert>
#include <cstring> // 注意memset是cstring里的
#include <algorithm>
#include "jsoncpp/json.h" // 在平台上，C++编译时默认包含此库

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

int lastplayer; //当前的最后一个出牌者 
short all_deck_maxlevel = MAX_STRAIGHT_LEVEL; //当前场上存在的最大的牌 
/**
 * 将Card变成Level
 */
constexpr Level card2level(Card card)
{
	return card / 4 + card / 53;
}


// 我的牌有哪些
set<Card> myCards;

// 地主被明示的牌有哪些
set<Card> landlordPublicCards;

// 大家从最开始到现在都出过什么
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];

// 大家还剩多少牌
short cardRemaining[PLAYER_COUNT] = { 20, 17, 17 };

// 我是几号玩家（0-地主，1-农民甲，2-农民乙）
int myPosition;

//已经使用过的牌
short used_card[MAX_LEVEL + 1] = {0};


int find_Max_Seq(short count[MAX_LEVEL + 1], int minlevel, int num){ //检查牌组最多能递增几个 
	int len = 0;
	for (unsigned c = minlevel; c <= MAX_STRAIGHT_LEVEL; c++){
		if (count[c] >= num && count[c] != 4){
			len++;
		}else{
			break;
		} 
	}		
	return len;
}


short find_all_deck_maxlevel(){ // 找到当前卡组的最大牌 不包括 2 小王 大王 
	Level i = MAX_STRAIGHT_LEVEL;
	for(; i >= 0; i--){
		if(used_card[i] != 4) return i;
	}
	return i;
}



// 牌的组合，用于计算牌型
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


	bool isBigCardcombo() const{//----------判断此牌是否为大牌，在农民配合时有用，判断队友是否有获得主动权的欲望
		if (comboType == CardComboType::SINGLE) 
		{
			if (comboLevel >= 11)//----------单牌大于=A
				return true;
			return false;
		}
		else if(comboType == CardComboType::PAIR||comboType == CardComboType::TRIPLET || comboType == CardComboType::TRIPLET1 || comboType == CardComboType::TRIPLET2) 
		{
			if (comboLevel >= 8)//----------多牌牌大于=J
				return true;
			return false;
		}
		if (comboType == CardComboType::STRAIGHT || comboType == CardComboType::STRAIGHT2)
			return false;//----------顺子能接就接
		else 
			return true;
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
	 
	 
	template <typename CARD_ITERATOR>
    CardCombo find_min_comb(CARD_ITERATOR begin, CARD_ITERATOR end) const// 找到最小牌组合  返回组合  非残局主动出牌阶段 
    {
		// 特判：空
		if (begin == end)
		{
			return CardCombo();
		} 
        
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
    	if(len3 >= 2){//plane 可以形成 飞机带大翼还没写 
    		usecard.clear();
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
			vector<Card> single_card; levelx = -1;  //找到单牌 
			for(;i < CARDS.size() && l > 0 && card2level(CARDS[i]) < all_deck_maxlevel; i++){
    			if(card2level(CARDS[i]) != levelx && COUNTS[card2level(CARDS[i])] == 1 && card2level(CARDS[i]) < all_deck_maxlevel){
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
		if(len2 >= 3){ //STRAIGHT2可以形成 
			usecard.clear();
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
    	if(len1 >= 5){ //STRAIGHT可以形成
    		usecard.clear();
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
		if(COUNTS[minlevel] == 3){
			usecard.clear();
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
				if(COUNTS[card2level(CARDS[j])] == 2 && card2level(CARDS[j]) < all_deck_maxlevel){
					usecard.push_back(CARDS[j]);usecard.push_back(CARDS[j + 1]);
					return CardCombo(usecard.begin(), usecard.end());
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
		else if(COUNTS[minlevel] == 2){
			usecard.clear();
			unsigned i = 0;
			usecard.push_back(CARDS[i]);i++; 
			usecard.push_back(CARDS[i]);i++;
			int t = 0;
			for(; i < CARDS.size(); i++){
				if(COUNTS[card2level(CARDS[i])] == 3 && card2level(CARDS[i]) < all_deck_maxlevel ){
					usecard.push_back(CARDS[i]);
					t++;
					if(t == 3) break;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		}
		else if(COUNTS[minlevel] == 1){
			usecard.clear();
			unsigned i = 0;
			usecard.push_back(CARDS[i]);i++; int t = 0;
			for(; i < CARDS.size(); i++){
				if(COUNTS[card2level(CARDS[i])] == 3 && card2level(CARDS[i]) < all_deck_maxlevel){
					usecard.push_back(CARDS[i]);
					t++;
					if(t == 3) break;
				}
			}
			return CardCombo(usecard.begin(), usecard.end());
		} 
	}
	//从当前结束是哥哥我写的
	
	//**************主动出牌
	// -----------------尽量不出单牌 若只剩单牌则从大到小出
	template <typename CARD_ITERATOR>
	CardCombo no_single_card(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		CardCombo last_single_cardcombo = CardCombo();
		CardCombo ans = find_min_comb(begin, end);
		while (ans.comboType == CardComboType::SINGLE ) {
			last_single_cardcombo = ans;
			CARD_ITERATOR second = begin;
			++second;
			if (second == end){
				return last_single_cardcombo;
			}
			ans = find_min_comb(++begin, end);
		}
		return ans;
	}

	//-------------------尽量不出单牌和对子 若只剩单牌和对子 则从大到小出对子和单牌
	template <typename CARD_ITERATOR>
	CardCombo no_single_pair_card(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		CardCombo last_single_cardcombo = CardCombo();
		CardCombo last_pair_cardcombo = CardCombo();
		CardCombo ans = find_min_comb(begin, end);
		while (ans.comboType == CardComboType::SINGLE || ans.comboType == CardComboType::PAIR) {
			if (ans.comboType == CardComboType::SINGLE)last_single_cardcombo = ans;
			else last_pair_cardcombo = ans;
			CARD_ITERATOR second = begin;
			if (ans.comboType == CardComboType::SINGLE)++second;
			else if (ans.comboType == CardComboType::PAIR) {
				++second; ++second;
			}
			if (second == end)
			{
				if (last_pair_cardcombo.comboType != CardComboType::PASS)
					return last_pair_cardcombo;
				else return last_single_cardcombo;
			}
			ans = find_min_comb(++begin, end);
		}
		return ans;
	}

	//-------------------出最小的一张牌
	template <typename CARD_ITERATOR>
	CardCombo min_single_card(CARD_ITERATOR begin, CARD_ITERATOR end) const {
		auto deck = vector<Card>(begin, end); // 手牌
		sort(deck.begin(), deck.end());
		vector<Card> anscard;
		anscard.push_back(deck[0]);
		return CardCombo(anscard.begin(), anscard.end());
	}


	//***************跟牌
	//-------------------出最大的一张或两张牌
	template <typename CARD_ITERATOR>
	CardCombo max_single_pair_card(CARD_ITERATOR begin, CARD_ITERATOR end, int n) const {
		auto deck = vector<Card>(begin, end); // 手牌
		short counts[MAX_LEVEL + 1] = {};

		// 先数一下手牌里每种牌有多少个
		for (Card c : deck)
			counts[card2level(c)]++;

		vector<Card> anscard;

		for (Level i = level_JOKER; i >= 0; --i) {
			if (counts[i] >= n && comboLevel<i) {
				for (Card c : deck) {
					if (card2level(c) == i) {
						anscard.push_back(c);
						if (anscard.size() == n) {
							return CardCombo(anscard.begin(), anscard.end());
						}
					}
				}
			}
		}
		return CardCombo();
	}

	//-------------------出最小的一张或两张牌
	template <typename CARD_ITERATOR>
	CardCombo follow_min_single_pair_card(CARD_ITERATOR begin, CARD_ITERATOR end, int n) const {
		auto deck = vector<Card>(begin, end); // 手牌
		short counts[MAX_LEVEL + 1] = {};

		// 先数一下手牌里每种牌有多少个
		for (Card c : deck)
			counts[card2level(c)]++;

		vector<Card> anscard;

		for (Level i = 0; i <= level_JOKER; ++i) {
			if (counts[i] >= n && comboLevel<i) {
				for (Card c : deck) {
					if (card2level(c) == i) {
						anscard.push_back(c);
						if (anscard.size() == n) {
							return CardCombo(anscard.begin(), anscard.end());
						}
					}
				}
			}
		}
		return CardCombo();
	}
	
	
	
	template <typename CARD_ITERATOR>
	CardCombo findFirstValid(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		all_deck_maxlevel = find_all_deck_maxlevel();
		// 现在打算从手牌中凑出同牌型的牌
		auto deck = vector<Card>(begin, end); // 手牌
		short counts[MAX_LEVEL + 1] = {};

		unsigned short kindCount = 0;

		// 先数一下手牌里每种牌有多少个
		for (Card c : deck)
			counts[card2level(c)]++;
		
		if (comboType == CardComboType::PASS) // 如果不需要大过谁，只需要随便出
		{
			//-------------------以下为终局策略
			if (myPosition == 0) {
				int cardCountMin = cardRemaining[1] >= cardRemaining[2] ? cardRemaining[2] : cardRemaining[1];
				int cardCountMax = cardRemaining[1] <= cardRemaining[2] ? cardRemaining[2] : cardRemaining[1];
				if (cardCountMin == 1 && cardCountMax >= 4) {// -----------------尽量不出单牌 若只剩单牌则从大到小出
					return no_single_card(begin, end);
				}
				else if (cardCountMin == 1 && cardCountMax < 4) {// -----------------尽量不出单牌和对子 若只剩单牌和对子 则从大到小出对子和单牌
					return no_single_pair_card(begin, end);
				}
				else if (cardCountMin == 2)
					return no_single_pair_card(begin, end);
			}
			else if (myPosition == 2) {
				if (cardRemaining[0] == 1)
					return no_single_card(begin, end);
				else if (cardRemaining[0] == 2)
					return no_single_pair_card(begin, end);
				else if (cardRemaining[1] <= 2 && cardRemaining[0] >= 3) {//--------------就出最小的一张牌
					return min_single_card(begin, end);
				}
			}
			else if (myPosition == 1) {
				if (cardRemaining[0] == 1 && cardRemaining[2] == 1) {
					return no_single_card(begin, end);
				}
				else if (cardRemaining[0] >= 2 && cardRemaining[2] == 1) {
					return min_single_card(begin, end);
				}
			}
			//------------------以上为终局策略
			return find_min_comb(begin, end);//哥哥自己的 
		}
		//-----------------终局策略
		//----------地主策略
		if (myPosition == 0) {
			if (cardRemaining[1] == 1) {
				//先炸一炸
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
				if (comboType == CardComboType::SINGLE) {
					return max_single_pair_card(begin, end, 1);
				}
				if (cardRemaining[2] <= 4) {
					if (comboType == CardComboType::PAIR) {
						return max_single_pair_card(begin, end, 2);
					}
				}
			}
			else if (cardRemaining[2] == 1) {
				//先炸一炸
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
				if (comboType == CardComboType::SINGLE) {
					return max_single_pair_card(begin, end, 1);
				}
				if (comboType == CardComboType::PAIR) {
					return max_single_pair_card(begin, end, 2);
				}
			}
		}
		//---------农民乙策略
		else if (myPosition == 2) {
			if (lastplayer == 1 && (cards.size() > cardRemaining[0]) return CardCombo();
			if (cardRemaining[0] == 1) {
				if (comboType == CardComboType::SINGLE) {//----------------出最大的一张牌

														 //先炸一炸
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

					return max_single_pair_card(begin, end, 1);
				}
			}
			else if (cardRemaining[0] == 2) {

				//先炸一炸
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

				if (comboType == CardComboType::SINGLE)
					return max_single_pair_card(begin, end, 1);
				else if (comboType == CardComboType::PAIR)
					return max_single_pair_card(begin, end, 2);
			}
			else if (cardRemaining[1] == 1) {
				if (comboType != CardComboType::SINGLE) {

					//先炸一炸
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

					if (comboType == CardComboType::PAIR) {
						return max_single_pair_card(begin, end, 2);
					}
				}

			}
		}
		//--------农民甲策略
		else if (myPosition == 1) {
			if (cardRemaining[0] == 1) {
				if (comboType == CardComboType::SINGLE) {//----------------出最大的一张牌

														 //先炸一炸
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

					return max_single_pair_card(begin, end, 1);
				}
			}
		}
        //-------农民配合——过牌
		if (myPosition != 0) {
			if (lastplayer > 0) {
				if (isBigCardcombo())//----------不压队友
					return CardCombo();
			}
		}
		if (myPosition == 1) {
			if (lastplayer > 0) {
				if (cardRemaining[2] < 7)//----------不压队友
					return CardCombo();
			}
		}
		// 然后先看一下是不是火箭，是的话就过
		if (comboType == CardComboType::ROCKET)
			return CardCombo();

		

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
				
			if(comboType == CardComboType::SINGLE){
				for(Level i = 1;;i++){
					int level = packs[0].level + i;
					if(level > MAX_LEVEL) break;
					if(comboType == CardComboType::SINGLE && myPosition > 0 && lastplayer > 0 && level >= 12) break;
					if(counts[level] != 1) continue;
					vector<Card> solve;
					for (Card c : deck){
						if(card2level(c) == level){
							solve.push_back(c);
							return CardCombo(solve.begin(), solve.end());
						}
					}
				}
			}
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
						
					if(comboType == CardComboType::PAIR && myPosition > 0 && lastplayer > 0 && level >= 11)//农民不压农民 
					    return CardCombo();
					
					if(comboType == CardComboType::SINGLE && myPosition > 0 && lastplayer > 0 && level >= 12)//农民不压农民 
					    return CardCombo();

					// 如果手牌中这种牌不够，就不用继续增了
					if (counts[level] < packs[j].count || counts[level] == 4)
						goto next;
				}

				{
					// 找到了合适的主牌，那么从牌呢？
					// 如果手牌的种类数不够，那从牌的种类数就不够，也不行
					if (kindCount < packs.size())
						continue;

					// 好终于可以了
					// 计算每种牌的要求数目吧
						bool has_perfect_card = 1;
						short requiredCounts[MAX_LEVEL + 1] = {};
						for (int j = 0; j < mainPackCount; j++)
							requiredCounts[packs[j].level + i] = packs[j].count;
						for (unsigned j = mainPackCount; j < packs.size(); j++)
						{
							Level k;
							for (k = 0; k <= MAX_LEVEL; k++)
							{
								if (requiredCounts[k] || counts[k] != packs[j].count || counts[k] == 4 || k >= all_deck_maxlevel)
									continue;
								requiredCounts[k] = packs[j].count;
								break;
							}
							if (k == MAX_LEVEL + 1){// 如果是都不符合要求……就不行了
							    has_perfect_card = 0;
							    memset(requiredCounts, 0, sizeof(requiredCounts));
								break;
							}          
						}
						if(has_perfect_card == 0){
							for (int j = 0; j < mainPackCount; j++)
								requiredCounts[packs[j].level + i] = packs[j].count;
							for (unsigned j = mainPackCount; j < packs.size(); j++)
							{
								Level k;
								for (k = 0; k <= MAX_LEVEL; k++)
								{
									if (requiredCounts[k] || counts[k] < packs[j].count || counts[k] == 4)
										continue;
									requiredCounts[k] = packs[j].count;
									break;
								}
								if (k == MAX_LEVEL + 1) // 如果是都不符合要求……就不行了
									goto next;
							}
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
		if(myPosition > 0 && lastplayer > 0) return CardCombo(); // 不炸自己人 
		
		for (Level i = 0; i < level_joker; i++)
			if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // 如果对方是炸弹，能炸的过才行
			{
				// 还真可以啊……
				Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
				return CardCombo(bomb, bomb + 4);
			}

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

// 当前要出的牌需要大过谁
CardCombo lastValidCombo;

namespace BotzoneIO
{
	using namespace std;
	void input()
	{
		// 读入输入（平台上的输入是单行）
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);

		// 首先处理第一回合，得知自己是谁、有哪些牌
		{
			auto firstRequest = input["requests"][0u]; // 下标需要是 unsigned，可以通过在数字后面加u来做到
			auto own = firstRequest["own"];
			auto llpublic = firstRequest["public"];
			auto history = firstRequest["history"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			for (unsigned i = 0; i < llpublic.size(); i++)
				landlordPublicCards.insert(llpublic[i].asInt());
			if (history[0u].size() == 0)
				if (history[1].size() == 0)
					myPosition = 0; // 上上家和上家都没出牌，说明是地主
				else
					myPosition = 1; // 上上家没出牌，但是上家出牌了，说明是农民甲
			else
				myPosition = 2; // 上上家出牌了，说明是农民乙
		}

		// history里第一项（上上家）和第二项（上家）分别是谁的决策
		int whoInHistory[] = { (myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT, (myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT };

		int turn = input["requests"].size();
		
		memset(used_card, 0, sizeof(used_card));//清空使用卡组 
		
		for (int i = 0; i < turn; i++)
		{
			// 逐次恢复局面到当前
			auto history = input["requests"][i]["history"]; // 每个历史中有上家和上上家出的牌
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p]; // 是谁出的牌
				auto playerAction = history[p]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举这个人出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是出的一张牌
					used_card[card2level((Card)card)] ++;
					playedCards.push_back(card);
				}
				whatTheyPlayed[player].push_back(playedCards); // 记录这段历史
				cardRemaining[player] -= playerAction.size();

				if (playerAction.size() == 0)
					howManyPass++;
				else{
					lastValidCombo = CardCombo(playedCards.begin(), playedCards.end()); lastplayer = whoInHistory[p];
				}
					
			}

			if (howManyPass == 2){
				lastValidCombo = CardCombo(); lastplayer = -1;
			}
				

			if (i < turn - 1)
			{
				// 还要恢复自己曾经出过的牌
				auto playerAction = input["responses"][i]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举自己出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是自己出的一张牌
					used_card[card2level((Card)card)] ++;
					myCards.erase(card); // 从自己手牌中删掉
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // 记录这段历史
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
		all_deck_maxlevel = find_all_deck_maxlevel();
	}

	/**
	 * 输出决策，begin是迭代器起点，end是迭代器终点
	 * CARD_ITERATOR是Card（即short）类型的迭代器
	 */
	template <typename CARD_ITERATOR>
	void output(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		Json::Value result, response(Json::arrayValue);
		for (; begin != end; begin++)
			response.append(*begin);
		result["response"] = response;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}
}

int main()
{
	BotzoneIO::input();

	// 做出决策（你只需修改以下部分）

	// findFirstValid 函数可以用作修改的起点
	CardCombo myAction = lastValidCombo.findFirstValid(myCards.begin(), myCards.end());

	// 是合法牌
	assert(myAction.comboType != CardComboType::INVALID);

	assert(
		// 在上家没过牌的时候过牌
		(lastValidCombo.comboType != CardComboType::PASS && myAction.comboType == CardComboType::PASS) ||
		// 在上家没过牌的时候出打得过的牌
		(lastValidCombo.comboType != CardComboType::PASS && lastValidCombo.canBeBeatenBy(myAction)) ||
		// 在上家过牌的时候出合法牌
		(lastValidCombo.comboType == CardComboType::PASS && myAction.comboType != CardComboType::INVALID)
	);

	// 决策结束，输出结果（你只需修改以上部分）

	BotzoneIO::output(myAction.cards.begin(), myAction.cards.end());
}
