#pragma once

#ifndef DEL_H_
#define DEL_H_

class Del
{
public:
	int indice_d ;
	int debut, fin;
	float courbure ;
	float orientation ;
	Del();
	Del(Del& );
	Del(const Del& );
	Del& operator=(Del &);
	Del& operator=(const Del &);
	virtual ~Del();
};


#endif /* DEL_H_ */