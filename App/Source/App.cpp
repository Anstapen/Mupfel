#include "Game.h"

int main()
{
	Game game;
	if (game.Init())
	{
		game.Run();
	}
	return 0;
}