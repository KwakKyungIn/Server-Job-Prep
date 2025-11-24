#pragma once

enum PlayerType
{
	PT_Knight = 1,
	PT_Archer = 2,
	PT_Mage = 3,
};
class Player
{
public:
	Player(int playerType) : _playerType(playerType) 
	{

	}
	virtual ~Player() 
	{
	
	}

public:
	int _playerType;
};

class Knight : public Player
{
	Knight() : Player(PT_Knight) 
	{
	}

};

class Archer : public Player
{
	Archer() : Player(PT_Archer) 
	{
	}
};

class Mage : public Player
{
	Mage() : Player(PT_Mage) 
	{
	}
};

