#include "StdAfx.h"
#include "angpt.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

/* FUNCTIONS */

void ins_ang_pt (ang_pt **id,ang_pt *ang)
{
	ang->next = *id;
	*id = ang;

	return;
}

ang_pt *new_ang_pt (SIZE_TYPEVAL x,SIZE_TYPEVAL y)
{  ang_pt 	*new_pt;

	new_pt = (ang_pt *) malloc(sizeof(ang_pt));
	new_pt->x = x;
	new_pt->y = y;
	new_pt->next = NULL;

	return(new_pt);
}

void destroy_all_ang_pt(ang_pt **first_id)
{
	
	while (*first_id != NULL)
		del_ang_pt(first_id);	
	return;
}


void del_ang_pt(ang_pt **id)
{
	ang_pt   *dead;

	dead = *id;
	*id = dead->next;

	free(dead);
	return;
}

void verbose_write_ang_pt(ang_pt ang,char *angout)
{
	FILE		*pmf;
	int		k,l;
	ang_pt	*car;


	pmf = fopen(angout,"w");

	car = &ang;
	k = l = 0;

	while (car != NULL)
	{
		if (car->y < MAX_SIZE_TYPEVAL)
			fprintf(pmf,"angular pt[%2d] = (%f,%f)\n",++k,car->x,car->y);
		else
			fprintf(pmf,"corner ln [%2d] = %f\n",++l,car->x);
		car = car->next;
	}

	fclose(pmf);

	return;

}

void write_ang_pt(ang_pt ang,char *angout)
{
	FILE		*pmf;
	int		k,l;
	ang_pt	*car;


	pmf = fopen(angout,"w");

	car = &ang;
	k = l = 0;

	while (car != NULL)
	{
		if (car->y < MAX_SIZE_TYPEVAL)
			fprintf(pmf,"p %d %f %f\n",++k,car->x,car->y);
		else
			fprintf(pmf,"l %d %f\n",++l,car->x);
		car = car->next;
	}

	fclose(pmf);
	printf("\nAngle Points saved successfully : %s\n",angout);
	return;

}

ang_pt *read_ang_pt(char *angin)
{
	FILE	*pmf;
	int	k,l;
	ang_pt	*size;
	SIZE_TYPEVAL    x,y;
	char	chkchr;

	size = NULL;

	pmf = fopen(angin,"r");

	k = l = 0;

	while (feof(pmf) == 0)
	{
		fscanf(pmf,"%c",&chkchr);
		if (chkchr == 'p')
			fscanf(pmf,"%d %f %f\n",&k,&x,&y);
		else if (chkchr == 'l')
		{
			fscanf(pmf,"%d %f\n",&l,&x);
			y = MAX_SIZE_TYPEVAL;
		}
		else
		{
			fprintf(stderr,"I can't read neither 'l' nor 'p' at the begin of line\n");
			exit(2);
		}
		ins_ang_pt (&size,new_ang_pt(x,y));
	}

	fclose(pmf);

	return(size);
}
ang_pt *read_ang_pt(const char *angin)
{
	FILE	*pmf;
	int	k,l;
	ang_pt	*size;
	SIZE_TYPEVAL    x,y;
	char	chkchr;

	size = NULL;

	pmf = fopen(angin,"r");

	k = l = 0;
	if(pmf==NULL){
		fprintf(stderr,"I can't the file %s\n",angin);
		system("pause");
			exit(2);
	}
	while (feof(pmf) == 0)
	{
		fscanf(pmf,"%c",&chkchr);
		if (chkchr == 'p')
			fscanf(pmf,"%d %f %f\n",&k,&x,&y);
		else if (chkchr == 'l')
		{
			fscanf(pmf,"%d %f\n",&l,&x);
			y = MAX_SIZE_TYPEVAL;
		}
		else
		{
			fprintf(stderr,"I can't read neither 'l' nor 'p' at the begin of line\n");
			exit(2);
		}
		ins_ang_pt (&size,new_ang_pt(x,y));
	}

	fclose(pmf);

	return(size);
}

