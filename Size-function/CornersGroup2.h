#pragma once
#include <vector>
#include "SizeFunction.h"

class CornersGroup2
{
	std::vector<SizeFunction> descripteur;

public:
	CornersGroup2(void);
	CornersGroup2(std::vector<std::string> fList);
	virtual ~CornersGroup2(void);
	double Pseudo_DistanceSF(CornersGroup2 b);
	void destroy();
	CornersGroup2(CornersGroup2& );
	CornersGroup2(const CornersGroup2& );
	CornersGroup2& operator=(CornersGroup2 &);
	CornersGroup2& operator=(const CornersGroup2 &);
};

