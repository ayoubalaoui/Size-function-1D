#include "StdAfx.h"
#include "Del.h"

Del::Del()
{
}

Del::~Del()
{

}

Del& Del::operator =(Del &Q)
{
	if(this!=&Q)
	{
		indice_d  = Q.indice_d;
		courbure = Q.courbure;
		orientation = Q.orientation;
		debut = Q.debut;
		fin = Q.fin;
	}
	return *this ;
}
Del& Del::operator =(const Del &Q)
{
	if(this!=&Q)
	{
		indice_d  = Q.indice_d;
		courbure = Q.courbure;
		orientation = Q.orientation;
		debut = Q.debut;
		fin = Q.fin;
	}
	return *this ;
}

Del::Del(Del &Q)
{
		indice_d  = Q.indice_d;
		courbure = Q.courbure;
		orientation = Q.orientation;
		debut = Q.debut;
		fin = Q.fin;
}
Del::Del(const Del &Q)
{
	indice_d  = Q.indice_d;
	courbure = Q.courbure;
	orientation = Q.orientation;
	debut = Q.debut;
	fin = Q.fin;
}
