#include "StdAfx.h"
#include "hausdorff.h"



/*	This function computes the Hausdorff distance between size functions.
	It gets in input the angular points of the two size functions; first of
	all it computes the distance between all the points of the 1st size
	function and all the points of the 2nd size function, storing them in
	the matrix diff[][].
	Then the function looks for the minimum distance, adds it to the Hausdorff
	distance, joining the two correspondent points; this process goes on until
	all the angular points of at least one of the two size function have been
	joined.
	The points that have not been joined are moved on the bisector of the I and
	III quadrant and these contributions are added to the Hausdorff distance.
	The function returns the hausdorff distance "haus_dist". */
float hausdorff(ang_pt *ang1in,ang_pt *ang2in)
{
	int	i,j,l,z;
	int	n_inf1;		/* number of 1st size function angular points to infinity */
	int	n_inf2;		/* number of 2nd size function angular points to infinity */
	int	dim1,dim2;
	int	*vect1;		/* if vect1[i] is 1, it means that the angular point i of
					the 1st size function has already been joined */
	int	*vect2;		/* if vect2[i] is 1, it means that the angular point i of
					the 2nd size function has already been joined */
	float	**diff;		/* this matirx stores the distances between the angular
					points of the 1st and the 2nd size functions */
	float	min;		/* distance between the joined angular points */
	double	haus_dist;	/* Hausdorff distance between the two size functions */
	ang_pt	*car1,*car2;
	Point	*ang1,*ang2;


	car1 = ang1in;
	dim1 = 0;
	while (car1 != NULL)
	{
		dim1++;
		car1 = car1->next;
	}

	car2 = ang2in;
	dim2 = 0;
	while (car2 != NULL)
	{
		dim2++;
		car2 = car2->next;
	}

	vect1=(int *)malloc(dim1*sizeof(int));
	for(i=0; i<dim1; i++)
		vect1[i] = 0;

	vect2=(int *)malloc(dim2*sizeof(int));
	for(j=0; j<dim2; j++)
		vect2[j] = 0;

	diff = (float **) malloc(dim1*sizeof(float *));
	for (i=0; i<dim1; i++)
		diff[i] = (float *) malloc(dim2*sizeof(float));
	for(i=0; i<dim1; i++)
		for(j=0; j<dim2; j++)
			diff[i][j] = FLT_MAX;


	/* We count the number of angular points to infinity of the 1st and the 2nd
		size function */
	ang1=(Point *)malloc(dim1*sizeof(Point));
	car1 = ang1in;
	i = dim1-1;
	j = 0;
	
	while (car1 != NULL)
	{
		if (car1->y == MAX_SIZE_TYPEVAL)
		{
			ang1[j].x = car1->x;
			ang1[j].y = car1->y;
			j++;
		}
		else
		{
			ang1[i].x = car1->x;
			ang1[i].y = car1->y;
			i--;
		}
		car1 = car1->next;
	}
	n_inf1 = j;

	ang2=(Point *)malloc(dim2*sizeof(Point));
	car2 = ang2in;
	i = dim2-1;
	j = 0;
	
	while (car2 != NULL)
	{
		if (car2->y == MAX_SIZE_TYPEVAL)
		{
			ang2[j].x = car2->x;
			ang2[j].y = car2->y;
			j++;
		}
		else
		{
			ang2[i].x = car2->x;
			ang2[i].y = car2->y;
			i--;
		}
		car2 = car2->next;
	}
	n_inf2 = j;

	for (i=0; i<dim1; i++)
		for (j=0; j<dim2; j++)
			if (i < n_inf1 && j < n_inf2)
				/* In this case the angular points are both to infinity, so we
					compute the difference between the abscissae of the points. */
				diff[i][j] = fabs((ang1+i)->x - (ang2+j)->x);
			else
			/*	In this case the angular point ang1 is not to infinity, so we
				compute the distance between ang1 and all the 2nd size function
				angular points which are not to infinity. */
				diff[i][j] = (float) DIST((double) (ang1+i)->x,(double) (ang1+i)->y,(double) (ang2+j)->x,(double) (ang2+j)->y);

	haus_dist = 0.0;
	do
	{
		/* We search for the minimum distance between the joined angular points */
		min = FLT_MAX;
		for(i=0; i<dim1; i++)
			for(j=0; j<dim2; j++)
				if (diff[i][j] < min)
				{
					min = diff[i][j];
					l = i;
					z = j;
				}
		
		if (min < FLT_MAX)
			haus_dist += min;

		/* If the angular points are not all joined */
		if (min != FLT_MAX)
		{
			for(i=0; i<dim1; i++)
			{
				diff[i][z] = FLT_MAX;
				vect2[z] = 1;
			}
			for(j=0; j<dim2; j++)
			{
				diff[l][j] = FLT_MAX;
				vect1[l] = 1;
			}
		}
	} while (min != FLT_MAX);	/* until all the angular points of at least one
											of the two size function are joined. */


	/* If some point in the 1st and in the 2nd size function have not still
		been joined and they are not points to infinite, their distances from
		the bisector are added to haus_dist */
	if (dim1 != dim2 || (dim1 == dim2 && n_inf1 != n_inf2))
	{
		for(i=0; i<dim1; i++)
			if (vect1[i] == 0)
				if ((ang1+i)->y != MAX_SIZE_TYPEVAL)
					haus_dist += ((ang1+i)->y - (ang1+i)->x) / SQUAREOF2;

		for(j=0; j<dim2; j++)
			if (vect2[j] == 0)
				if ((ang2+j)->y != MAX_SIZE_TYPEVAL)
					haus_dist += ((ang2+j)->y - (ang2+j)->x) / SQUAREOF2;
	}

	for (i=0; i<dim1; i++)
		free(diff[i]);
	free(diff);
	free(vect2);
	free(vect1);
	free(ang1);
	free(ang2);

	return (haus_dist);

}	/* end hausdorff */

