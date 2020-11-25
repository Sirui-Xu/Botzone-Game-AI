#include <iostream>
#include <set>
#include <string>
#include <cassert>
#include <cstring> // ע��memset��cstring���
#include <algorithm>
#include "jsoncpp/json.h" // ��ƽ̨�ϣ�C++����ʱĬ�ϰ����˿�

using std::vector;
using std::sort;
using std::unique;
using std::set;
using std::string;

constexpr int PLAYER_COUNT = 3;

enum class CardComboType
{
	PASS, // ��
	SINGLE, // ����
	PAIR, // ����
	STRAIGHT, // ˳��
	STRAIGHT2, // ˫˳
	TRIPLET, // ����
	TRIPLET1, // ����һ
	TRIPLET2, // ������
	BOMB, // ը��
	QUADRUPLE2, // �Ĵ�����ֻ��
	QUADRUPLE4, // �Ĵ������ԣ�
	PLANE, // �ɻ�
	PLANE1, // �ɻ���С��
	PLANE2, // �ɻ�������
	SSHUTTLE, // ����ɻ�
	SSHUTTLE2, // ����ɻ���С��
	SSHUTTLE4, // ����ɻ�������
	ROCKET, // ���
	INVALID // �Ƿ�����
};

int cardComboScores[] = {
	0, // ��
	1, // ����
	2, // ����
	6, // ˳��
	6, // ˫˳
	4, // ����
	4, // ����һ
	4, // ������
	10, // ը��
	8, // �Ĵ�����ֻ��
	8, // �Ĵ������ԣ�
	8, // �ɻ�
	8, // �ɻ���С��
	8, // �ɻ�������
	10, // ����ɻ�����Ҫ���У�����Ϊ10�֣�����Ϊ20�֣�
	10, // ����ɻ���С��
	10, // ����ɻ�������
	16, // ���
	0 // �Ƿ�����
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

// ��0~53��54��������ʾΨһ��һ����
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// ������0~53��54��������ʾΨһ���ƣ�
// ���ﻹ����һ����ű�ʾ�ƵĴ�С�����ܻ�ɫ�����Ա�Ƚϣ������ȼ���Level��
// ��Ӧ��ϵ���£�
// 3 4 5 6 7 8 9 10	J Q K	A	2	С��	����
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

int lastplayer; //��ǰ�����һ�������� 
short all_deck_maxlevel = MAX_STRAIGHT_LEVEL; //��ǰ���ϴ��ڵ������� 
/**
 * ��Card���Level
 */
constexpr Level card2level(Card card)
{
	return card / 4 + card / 53;
}


// �ҵ�������Щ
set<Card> myCards;

// ��������ʾ��������Щ
set<Card> landlordPublicCards;

// ��Ҵ��ʼ�����ڶ�����ʲô
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];

// ��һ�ʣ������
short cardRemaining[PLAYER_COUNT] = { 20, 17, 17 };

// ���Ǽ�����ң�0-������1-ũ��ף�2-ũ���ң�
int myPosition;

//�Ѿ�ʹ�ù�����
short used_card[MAX_LEVEL + 1] = {0};


int find_Max_Seq(short count[MAX_LEVEL + 1], int minlevel, int num){ //�����������ܵ������� 
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


short find_all_deck_maxlevel(){ // �ҵ���ǰ���������� ������ 2 С�� ���� 
	Level i = MAX_STRAIGHT_LEVEL;
	for(; i >= 0; i--){
		if(used_card[i] != 4) return i;
	}
	return i;
}



// �Ƶ���ϣ����ڼ�������
struct CardCombo
{
	// ��ʾͬ�ȼ������ж�����
	// �ᰴ�����Ӵ�С���ȼ��Ӵ�С����
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
	vector<Card> cards; // ԭʼ���ƣ�δ����
	vector<CardPack> packs; // ����Ŀ�ʹ�С���������
	CardComboType comboType; // ���������
	Level comboLevel = 0; // ����Ĵ�С��

	/**
	 * ����������CardPack�ݼ��˼���
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
	* �������������ֵܷ�ʱ���Ȩ��
	*/
	int getWeight() const
	{
		if (comboType == CardComboType::SSHUTTLE ||
			comboType == CardComboType::SSHUTTLE2 ||
			comboType == CardComboType::SSHUTTLE4)
			return cardComboScores[(int)comboType] + (findMaxSeq() > 2) * 10;
		return cardComboScores[(int)comboType];
	}

	// ����һ��������
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	 * ͨ��Card����short�����͵ĵ���������һ������
	 * ����������ͺʹ�С���
	 * ��������û���ظ����֣����ظ���Card��
	 */
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// ���У���
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// ÿ�����ж��ٸ�
		short counts[MAX_LEVEL + 1] = {};

		// ͬ���Ƶ��������ж��ٸ����š����ӡ�������������
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

		// ���������������ǿ��ԱȽϴ�С��
		comboLevel = packs[0].level;

		// ��������
		// ���� ͬ���Ƶ����� �м��� ���з���
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // ֻ��һ����
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// ֻ�����ɵ���
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
				// ֻ�����ɶ���
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
				// ֻ����������
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
				// ֻ����������
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
		case 2: // ��������
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// ��������
				if (kindOfCountOfCount[0] == 1)
				{
					// ����һ
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
					// ������
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
				// ��������
				if (kindOfCountOfCount[0] == 1)
				{
					// ��������ֻ * n
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
					// ���������� * n
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


	bool isBigCardcombo() const{//----------�жϴ����Ƿ�Ϊ���ƣ���ũ�����ʱ���ã��ж϶����Ƿ��л������Ȩ������
		if (comboType == CardComboType::SINGLE) 
		{
			if (comboLevel >= 11)//----------���ƴ���=A
				return true;
			return false;
		}
		else if(comboType == CardComboType::PAIR||comboType == CardComboType::TRIPLET || comboType == CardComboType::TRIPLET1 || comboType == CardComboType::TRIPLET2) 
		{
			if (comboLevel >= 8)//----------�����ƴ���=J
				return true;
			return false;
		}
		if (comboType == CardComboType::STRAIGHT || comboType == CardComboType::STRAIGHT2)
			return false;//----------˳���ܽӾͽ�
		else 
			return true;
	}
	
	
	/**
	 * �ж�ָ�������ܷ�����ǰ���飨������������ǹ��Ƶ��������
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
	 * ��ָ��������Ѱ�ҵ�һ���ܴ����ǰ���������
	 * ��������Ļ�ֻ����һ��
	 * ����������򷵻�һ��PASS������
	 */
	 
	 
	template <typename CARD_ITERATOR>
    CardCombo find_min_comb(CARD_ITERATOR begin, CARD_ITERATOR end) const// �ҵ���С�����  �������  �ǲо��������ƽ׶� 
    {
		// ���У���
		if (begin == end)
		{
			return CardCombo();
		} 
        
		// ÿ�����ж��ٸ�
		short COUNTS[MAX_LEVEL + 1] = {};

		vector<Card> CARDS(begin, end);
		vector<Card> usecard;//Ҫ���Ŀ��� 
		sort(CARDS.begin(), CARDS.end());
		for (Card c : CARDS)
			COUNTS[card2level(c)]++;
	
		int minlevel = card2level(CARDS[0]);
		int len1 = find_Max_Seq(COUNTS, minlevel, 1);// STRAIGHT�ĳ���
		int len2 = find_Max_Seq(COUNTS, minlevel, 2);// STRAIGHT2�ĳ���
		int len3 = find_Max_Seq(COUNTS, minlevel, 3);// plane�ĳ��� 
		CARD_ITERATOR second = begin;
		if(COUNTS[minlevel] == 4){//��С�������ը�� ��ʹ�ñȽϺð� 
			second ++; second ++; second ++; second ++;
			CardCombo new_CARDS = find_min_comb(second, end);//��������� 
			if(new_CARDS.comboType == CardComboType::PASS) {
				return CardCombo(begin, end);          //ֻ��ը����� 
			} 
			else {
				return new_CARDS; //���߷���֮������ 
			}
		}
    	if(len3 >= 2){//plane �����γ� �ɻ�������ûд 
    		usecard.clear();
			int levelx = minlevel;//��ǰlevel 
    		int t = 0;//������ʹ���������� 
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
			vector<Card> single_card; levelx = -1;  //�ҵ����� 
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
		if(len2 >= 3){ //STRAIGHT2�����γ� 
			usecard.clear();
    		int levelx = minlevel;//��ǰlevel 
    		int t = 0;//������ʹ����һ���� 
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
    	if(len1 >= 5){ //STRAIGHT�����γ�
    		usecard.clear();
    		int levelx = minlevel;//��ǰlevel 
    		int t = 0; 
    		for(unsigned i = 0; i < CARDS.size() && len1 > 0; i++){
    			if(t == 1){ //ֻ��Ҫ1���� 
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
	//�ӵ�ǰ�����Ǹ����д��
	
	//**************��������
	// -----------------������������ ��ֻʣ������Ӵ�С��
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

	//-------------------�����������ƺͶ��� ��ֻʣ���ƺͶ��� ��Ӵ�С�����Ӻ͵���
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

	//-------------------����С��һ����
	template <typename CARD_ITERATOR>
	CardCombo min_single_card(CARD_ITERATOR begin, CARD_ITERATOR end) const {
		auto deck = vector<Card>(begin, end); // ����
		sort(deck.begin(), deck.end());
		vector<Card> anscard;
		anscard.push_back(deck[0]);
		return CardCombo(anscard.begin(), anscard.end());
	}


	//***************����
	//-------------------������һ�Ż�������
	template <typename CARD_ITERATOR>
	CardCombo max_single_pair_card(CARD_ITERATOR begin, CARD_ITERATOR end, int n) const {
		auto deck = vector<Card>(begin, end); // ����
		short counts[MAX_LEVEL + 1] = {};

		// ����һ��������ÿ�����ж��ٸ�
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

	//-------------------����С��һ�Ż�������
	template <typename CARD_ITERATOR>
	CardCombo follow_min_single_pair_card(CARD_ITERATOR begin, CARD_ITERATOR end, int n) const {
		auto deck = vector<Card>(begin, end); // ����
		short counts[MAX_LEVEL + 1] = {};

		// ����һ��������ÿ�����ж��ٸ�
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
		// ���ڴ���������дճ�ͬ���͵���
		auto deck = vector<Card>(begin, end); // ����
		short counts[MAX_LEVEL + 1] = {};

		unsigned short kindCount = 0;

		// ����һ��������ÿ�����ж��ٸ�
		for (Card c : deck)
			counts[card2level(c)]++;
		
		if (comboType == CardComboType::PASS) // �������Ҫ���˭��ֻ��Ҫ����
		{
			//-------------------����Ϊ�վֲ���
			if (myPosition == 0) {
				int cardCountMin = cardRemaining[1] >= cardRemaining[2] ? cardRemaining[2] : cardRemaining[1];
				int cardCountMax = cardRemaining[1] <= cardRemaining[2] ? cardRemaining[2] : cardRemaining[1];
				if (cardCountMin == 1 && cardCountMax >= 4) {// -----------------������������ ��ֻʣ������Ӵ�С��
					return no_single_card(begin, end);
				}
				else if (cardCountMin == 1 && cardCountMax < 4) {// -----------------�����������ƺͶ��� ��ֻʣ���ƺͶ��� ��Ӵ�С�����Ӻ͵���
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
				else if (cardRemaining[1] <= 2 && cardRemaining[0] >= 3) {//--------------�ͳ���С��һ����
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
			//------------------����Ϊ�վֲ���
			return find_min_comb(begin, end);//����Լ��� 
		}
		//-----------------�վֲ���
		//----------��������
		if (myPosition == 0) {
			if (cardRemaining[1] == 1) {
				//��ըһը
				for (Level i = 0; i < level_joker; i++)
					if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
					{
						// ������԰�����
						Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
						return CardCombo(bomb, bomb + 4);
					}

				// ��û�л����
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
				//��ըһը
				for (Level i = 0; i < level_joker; i++)
					if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
					{
						// ������԰�����
						Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
						return CardCombo(bomb, bomb + 4);
					}

				// ��û�л����
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
		//---------ũ���Ҳ���
		else if (myPosition == 2) {
			if (lastplayer == 1 && (cards.size() > cardRemaining[0]) return CardCombo();
			if (cardRemaining[0] == 1) {
				if (comboType == CardComboType::SINGLE) {//----------------������һ����

														 //��ըһը
					for (Level i = 0; i < level_joker; i++)
						if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
						{
							// ������԰�����
							Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
							return CardCombo(bomb, bomb + 4);
						}

					// ��û�л����
					if (counts[level_joker] + counts[level_JOKER] == 2)
					{
						Card rocket[] = { card_joker, card_JOKER };
						return CardCombo(rocket, rocket + 2);
					}

					return max_single_pair_card(begin, end, 1);
				}
			}
			else if (cardRemaining[0] == 2) {

				//��ըһը
				for (Level i = 0; i < level_joker; i++)
					if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
					{
						// ������԰�����
						Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
						return CardCombo(bomb, bomb + 4);
					}

				// ��û�л����
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

					//��ըһը
					for (Level i = 0; i < level_joker; i++)
						if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
						{
							// ������԰�����
							Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
							return CardCombo(bomb, bomb + 4);
						}

					// ��û�л����
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
		//--------ũ��ײ���
		else if (myPosition == 1) {
			if (cardRemaining[0] == 1) {
				if (comboType == CardComboType::SINGLE) {//----------------������һ����

														 //��ըһը
					for (Level i = 0; i < level_joker; i++)
						if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
						{
							// ������԰�����
							Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
							return CardCombo(bomb, bomb + 4);
						}

					// ��û�л����
					if (counts[level_joker] + counts[level_JOKER] == 2)
					{
						Card rocket[] = { card_joker, card_JOKER };
						return CardCombo(rocket, rocket + 2);
					}

					return max_single_pair_card(begin, end, 1);
				}
			}
		}
        //-------ũ����ϡ�������
		if (myPosition != 0) {
			if (lastplayer > 0) {
				if (isBigCardcombo())//----------��ѹ����
					return CardCombo();
			}
		}
		if (myPosition == 1) {
			if (lastplayer > 0) {
				if (cardRemaining[2] < 7)//----------��ѹ����
					return CardCombo();
			}
		}
		// Ȼ���ȿ�һ���ǲ��ǻ�����ǵĻ��͹�
		if (comboType == CardComboType::ROCKET)
			return CardCombo();

		

		// ������������ã�ֱ�Ӳ��ô��ˣ������ܲ���ը��
		if (deck.size() < cards.size())
			goto failure;

		// ����һ���������ж�������
		for (short c : counts)
			if (c)
				kindCount++;

		// ���򲻶�����ǰ��������ƣ������ܲ����ҵ�ƥ�������
		{
			// ��ʼ��������
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
			for (Level i = 1; ; i++) // �������
			{
				for (int j = 0; j < mainPackCount; j++)
				{
					int level = packs[j].level + i;

					// �����������͵����Ʋ��ܵ�2�����������͵����Ʋ��ܵ�С�������ŵ����Ʋ��ܳ�������
					if ((comboType == CardComboType::SINGLE && level > MAX_LEVEL) ||
						(isSequential && level > MAX_STRAIGHT_LEVEL) ||
						(comboType != CardComboType::SINGLE && !isSequential && level >= level_joker))
						goto failure;
						
					if(comboType == CardComboType::PAIR && myPosition > 0 && lastplayer > 0 && level >= 11)//ũ��ѹũ�� 
					    return CardCombo();
					
					if(comboType == CardComboType::SINGLE && myPosition > 0 && lastplayer > 0 && level >= 12)//ũ��ѹũ�� 
					    return CardCombo();

					// ��������������Ʋ������Ͳ��ü�������
					if (counts[level] < packs[j].count || counts[level] == 4)
						goto next;
				}

				{
					// �ҵ��˺��ʵ����ƣ���ô�����أ�
					// ������Ƶ��������������Ǵ��Ƶ��������Ͳ�����Ҳ����
					if (kindCount < packs.size())
						continue;

					// �����ڿ�����
					// ����ÿ���Ƶ�Ҫ����Ŀ��
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
							if (k == MAX_LEVEL + 1){// ����Ƕ�������Ҫ�󡭡��Ͳ�����
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
								if (k == MAX_LEVEL + 1) // ����Ƕ�������Ҫ�󡭡��Ͳ�����
									goto next;
							}
						}

					
					// ��ʼ������
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
				; // ������
			}
		}
	failure:
		// ʵ���Ҳ�����
		// ���һ���ܲ���ը��
		if(myPosition > 0 && lastplayer > 0) return CardCombo(); // ��ը�Լ��� 
		
		for (Level i = 0; i < level_joker; i++)
			if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
			{
				// ������԰�����
				Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
				return CardCombo(bomb, bomb + 4);
			}

		if (counts[level_joker] + counts[level_JOKER] == 2)
		{
			Card rocket[] = { card_joker, card_JOKER };
			return CardCombo(rocket, rocket + 2);
		}

		// ����
		return CardCombo();
	}

	void debugPrint()
	{
#ifndef _BOTZONE_ONLINE
		std::cout << "��" << cardComboStrings[(int)comboType] <<
			"��" << cards.size() << "�ţ���С��" << comboLevel << "��";
#endif
	}
};

// ��ǰҪ��������Ҫ���˭
CardCombo lastValidCombo;

namespace BotzoneIO
{
	using namespace std;
	void input()
	{
		// �������루ƽ̨�ϵ������ǵ��У�
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);

		// ���ȴ����һ�غϣ���֪�Լ���˭������Щ��
		{
			auto firstRequest = input["requests"][0u]; // �±���Ҫ�� unsigned������ͨ�������ֺ����u������
			auto own = firstRequest["own"];
			auto llpublic = firstRequest["public"];
			auto history = firstRequest["history"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			for (unsigned i = 0; i < llpublic.size(); i++)
				landlordPublicCards.insert(llpublic[i].asInt());
			if (history[0u].size() == 0)
				if (history[1].size() == 0)
					myPosition = 0; // ���ϼҺ��ϼҶ�û���ƣ�˵���ǵ���
				else
					myPosition = 1; // ���ϼ�û���ƣ������ϼҳ����ˣ�˵����ũ���
			else
				myPosition = 2; // ���ϼҳ����ˣ�˵����ũ����
		}

		// history���һ����ϼң��͵ڶ���ϼң��ֱ���˭�ľ���
		int whoInHistory[] = { (myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT, (myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT };

		int turn = input["requests"].size();
		
		memset(used_card, 0, sizeof(used_card));//���ʹ�ÿ��� 
		
		for (int i = 0; i < turn; i++)
		{
			// ��λָ����浽��ǰ
			auto history = input["requests"][i]["history"]; // ÿ����ʷ�����ϼҺ����ϼҳ�����
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p]; // ��˭������
				auto playerAction = history[p]; // ������Щ��
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // ѭ��ö������˳���������
				{
					int card = playerAction[_].asInt(); // �����ǳ���һ����
					used_card[card2level((Card)card)] ++;
					playedCards.push_back(card);
				}
				whatTheyPlayed[player].push_back(playedCards); // ��¼�����ʷ
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
				// ��Ҫ�ָ��Լ�������������
				auto playerAction = input["responses"][i]; // ������Щ��
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // ѭ��ö���Լ�����������
				{
					int card = playerAction[_].asInt(); // �������Լ�����һ����
					used_card[card2level((Card)card)] ++;
					myCards.erase(card); // ���Լ�������ɾ��
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // ��¼�����ʷ
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
		all_deck_maxlevel = find_all_deck_maxlevel();
	}

	/**
	 * ������ߣ�begin�ǵ�������㣬end�ǵ������յ�
	 * CARD_ITERATOR��Card����short�����͵ĵ�����
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

	// �������ߣ���ֻ���޸����²��֣�

	// findFirstValid �������������޸ĵ����
	CardCombo myAction = lastValidCombo.findFirstValid(myCards.begin(), myCards.end());

	// �ǺϷ���
	assert(myAction.comboType != CardComboType::INVALID);

	assert(
		// ���ϼ�û���Ƶ�ʱ�����
		(lastValidCombo.comboType != CardComboType::PASS && myAction.comboType == CardComboType::PASS) ||
		// ���ϼ�û���Ƶ�ʱ�����ù�����
		(lastValidCombo.comboType != CardComboType::PASS && lastValidCombo.canBeBeatenBy(myAction)) ||
		// ���ϼҹ��Ƶ�ʱ����Ϸ���
		(lastValidCombo.comboType == CardComboType::PASS && myAction.comboType != CardComboType::INVALID)
	);

	// ���߽���������������ֻ���޸����ϲ��֣�

	BotzoneIO::output(myAction.cards.begin(), myAction.cards.end());
}
