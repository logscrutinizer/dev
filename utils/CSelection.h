/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CSELECTION
#define CSELECTION

/***********************************************************************************************************************
*   CSelection
***********************************************************************************************************************/
class CSelection
{
public:
    CSelection() : row(0), startCol(0), endCol(0) {}
    CSelection(int row, int startC, int endC) : row(row), startCol(startC), endCol(endC) {}

    int row;
    int startCol;
    int endCol;
};

#endif
