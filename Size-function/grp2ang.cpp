#include "StdAfx.h"
#include <stdlib.h>
#include <float.h>

#include "grp2ang.h"

/* FUNCTIONS */

ang_pt *size_deltared_graph(Graph G)
{
	D_graph	H;
	D_node	**order_array;
	D_node	*actual_node;
	Edge		*edge_to_erase;
	ang_pt	*size;
	int		i;
	SIZE_TYPEVAL	x,y;

	size=NULL;
	H = delta_star_reduction(G);
	order_array = H.order_array;

	for(i=H.n_nodes;i>0;i--)
	{
		if (order_array[i-1] != NULL)
		{
			actual_node = order_array[i-1];
			if (actual_node->l_adj_low == NULL)
			{
				x = actual_node->val;
				while (actual_node->l_adj_low == NULL &&
						actual_node->l_adj_high != NULL)
				{
					edge_to_erase = actual_node->l_adj_high->edge;
					actual_node = edge_to_erase->high.to;
					erase_Edge(edge_to_erase);
				}
				y = (actual_node->l_adj_low != NULL) ?
						actual_node->val : MAX_SIZE_TYPEVAL;
				ins_ang_pt (&size,new_ang_pt(x,y));
			}
		}
	}
	free(H.first);
	free(H.order_array);
	return(size);
}

ang_pt *size_deltared_P_graph(P_node *G)
{
	ang_pt 		*size;
	SIZE_TYPEVAL	x,y;
	P_node		**min_array;
	P_node		*sadle_prev,*sadle_next;
	int 		n_min,i;

	size = NULL;
	purge_plain_graph(G);
	find_min(G,&min_array,&n_min);
	quick_sort_plain(min_array,0,n_min-1);

	for (i=0;i<n_min;i++)
	{
		x = min_array[i]->val;
		sadle_prev = climb_prev(min_array[i]);
		sadle_next = climb_next(min_array[i]);
		if (sadle_prev != sadle_next)
		{
			if (sadle_prev->val<=sadle_next->val)
			{
				y = sadle_prev->val;
				sadle_next->prev = sadle_prev->prev;
				sadle_prev->prev->next = sadle_next;
				free(sadle_prev);
			}
			else
			{
				y = sadle_next->val;
				sadle_prev->next = sadle_next->next;
				sadle_next->next->prev = sadle_prev;
				free(sadle_next);
			}
		}
		else
			y = MAX_SIZE_TYPEVAL;

		ins_ang_pt (&size,new_ang_pt(x,y));

		free(min_array[i]);
	}

	free(min_array);

	return(size);


}

