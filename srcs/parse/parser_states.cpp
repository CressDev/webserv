#include "Parser.hpp"
#include <iostream>

int getState(int prev, int pos)
{
	static int tokens[][11] = {
		{S_ERR, S_SER, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR}, //  0 INI
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR}, //  1 Error
		{S_ERR, S_ERR, S_SOP, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR}, //  2 SERVER
		{S_ERR, S_ERR, S_ERR, S_ERR, S_LOC, S_ERR, S_ERR, S_ERR, S_KEY, S_ERR, S_ERR}, //  3 SERVER_OP
		{S_ERR, S_SER, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR}, //  4 SERVER_CL
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_LUR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR}, //  5 LOCATION
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_LOP, S_ERR, S_ERR, S_ERR, S_ERR}, //  6 LOCATION_URI
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_LCL, S_KEY, S_ERR, S_ERR}, //  7 LOCATION_OP
		{S_ERR, S_ERR, S_ERR, S_SCL, S_LOC, S_ERR, S_ERR, S_ERR, S_KEY, S_ERR, S_ERR}, //  8 LOCATION_CL
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_PAR, S_ERR}, //  9 KEYWORD
		{S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_PAR, S_SEM}, // 10 PARAMETER
		{S_ERR, S_ERR, S_ERR, S_SCL, S_LOC, S_ERR, S_ERR, S_LCL, S_KEY, S_ERR, S_ERR}, // 11 SEMICOLON
	};
	return tokens[prev][pos];
}

int	isConfigWord(std::string& token)
{
	for (size_t i = 0; i < 15; i++)
	{
		if (token == configkeys[i])
			return 1;
	}
	return 0;
}

int	Parser::getIPV4State(int prev, int pos)
{
	static int tokens[][8] = {
		{IP_ER, IP_00, IP_01, IP_02, IP_NU, IP_NU, IP_ER, IP_ER}, //  0 INI
		{IP_ER, IP_ER, IP_ER, IP_ER, IP_ER, IP_ER, IP_PO, IP_ER}, // IP_00
		{IP_ER, IP_01, IP_01, IP_01, IP_01, IP_01, IP_PO, IP_01}, // IP_01
		{IP_ER, IP_01, IP_01, IP_01, IP_01, IP_05, IP_PO, IP_00}, // IP_02
		{IP_ER, IP_ER, IP_ER, IP_ER, IP_ER, IP_ER, IP_ER, IP_ER}, // IP_03
		{IP_ER, IP_00, IP_00, IP_00, IP_00, IP_00, IP_PO, IP_ER}, // IP_05
		{IP_ER, IP_00, IP_01, IP_02, IP_NU, IP_NU, IP_ER, IP_NU}, // IP_PO
		{IP_ER, IP_00, IP_00, IP_00, IP_00, IP_00, IP_PO, IP_00} // IP_NU
	};
	return tokens[prev][pos];
}

int	Parser::getClientMaxBodySizeState(int prev, int pos)
{
	static int tokens[][3] = {
		{BD_ERR, BD_NUM, BD_ERR}, // 0 INI
		{BD_ERR, BD_NUM, BD_CHR}, // 1 BD_NUM
		{BD_ERR, BD_ERR, BD_ERR}  // 2 BD_CHR
	};
	return tokens[prev][pos];
}

int		Parser::getServerNameState(int prev, int pos)
{
	static int	matrix[][7] = {
		{1, 1, 2, 2, 2, 2, 1},
		{1, 1, 1, 1, 1, 1, 1}, //error
		{1, 3, 2, 2, 2, 2, 1}, //cualquier caracter
		{1, 1, 4, 2, 2, 2, 1}, //.
		{1, 3, 2, 5, 2, 2, 1}, //c
		{1, 3, 2, 2, 6, 2, 1}, //o
		{1, 3, 2, 2, 2, 2, 1}, //m
	};
	return (matrix[prev][pos]);
}
