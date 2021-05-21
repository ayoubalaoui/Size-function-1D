#pragma once
#include <string>
#include <vector>
#include "angpt.h"
class CornersGroup
{
	std::vector<ang_pt *> descripteur;

public:
	CornersGroup(void);
	CornersGroup(std::vector<std::string> fList);
	//void loadFromDirectory(const std::string &f);
	virtual ~CornersGroup(void);
	void destroy();
	double DistanceSF(const CornersGroup &b);
	double Pseudo_DistanceSF(const CornersGroup &b);
	CornersGroup(CornersGroup& );
	CornersGroup(const CornersGroup& );
	CornersGroup& operator=(CornersGroup &);
	CornersGroup& operator=(const CornersGroup &);
	
};

