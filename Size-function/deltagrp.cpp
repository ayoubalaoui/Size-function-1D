#include "StdAfx.h"
#include "deltagrp.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "grp2ang.h"

Graph new_graph(int n_nodes)
{
	Graph 	G;
	int		i;

	G.first = (Node *) malloc(n_nodes * sizeof(Node));
	G.n_nodes = n_nodes;
	for (i=0;i<n_nodes;i++)
	{
		G.first[i].val=(float)0.0;
		G.first[i].visit=0;
		G.first[i].l_adj=NULL;
	}

	return(G);
}

void add_new_edge_graph(Node *from,Node *to)
{
	L_node 	*new_edge;

	new_edge = (L_node *) malloc(sizeof(L_node));
	new_edge->p_node = to;
	new_edge->next = from->l_adj;
	from->l_adj = new_edge;
}

void erase_edge_graph(L_node **prev_id)
{
	L_node	*dead;

	if (*prev_id != NULL)
	{
		dead = *prev_id;
		*prev_id = (*prev_id)->next;
		free(dead);
	}

	return;
}

void insert_new_edge_graph(Node *to,L_node **prev_id)
{
	L_node	*new_edge;

	new_edge = (L_node *) malloc(sizeof(L_node));
	new_edge->p_node = to;
	new_edge->next = *prev_id;
	*prev_id = new_edge;
	return;
}

void insert_edge_graph(L_node *who,L_node **prev_id)
{
	if (who != NULL)
		if (*prev_id != NULL)
		{
			who->next = *prev_id;
			*prev_id = who;
		}

	return;
}

void purge_graph(Graph G)
{
	int		i;
	Node		*first=G.first;
	L_node   **car,**tmp;

	for(i=0;i<G.n_nodes;i++)
		first[i].visit = 0;
	for(i=0;i<G.n_nodes;i++)
	{
		car = &(first[i].l_adj);
		while (*car != NULL)
		{
			tmp = &((*car)->next);
			if ((*car)->p_node->visit != i + 1)
				(*car)->p_node->visit = i + 1;
			else
				erase_edge_graph(car);
			car = tmp;
		}
	}

	return;
}

void destroy_graph(Graph G)
{
	int		i;
	
	for(i=0;i<G.n_nodes;i++)
	{
		while (G.first[i].l_adj != NULL)
			erase_edge_graph(&(G.first[i].l_adj));
	}
	free(G.first);
	return;
}

Graph read_file_graph(char *filein)
{
	FILE		*pmf;
	int		k;
	int		m,pino;
	L_node	**car;
	Graph		G;
	char		key;

	pmf = fopen(filein,"r");
	/*fscanf(pmf,"%d\n",&(G.n_nodes)); */

	fscanf(pmf,"%d\n",&pino);
	G.n_nodes=pino;

	G.first = (Node *) malloc(pino*sizeof(Node));

	for (k=0; k<G.n_nodes; k++)
	{
		G.first[k].visit = 0;
		fscanf(pmf,"%f\n",&(G.first[k].val));
		car = &(G.first[k].l_adj);

		while ((key=getc(pmf)) != '/')
		{
			fscanf(pmf,"%d",&m);
			*car = (L_node *) malloc(sizeof(L_node));
			(*car)->p_node = G.first + ((int) m);
		   	car = &((*car)->next);

		}
		*car = NULL;
	}

	fclose(pmf);
	printf("\nGraph loaded successfully : %s\n",filein);
	return (G);

}

Graph random_graph(int n_nodes,int Max_out)
{

	Graph 	G;
	int		k,h,n_new,new_edge;
	L_node	**car;

	G.n_nodes=n_nodes;
	G.first = (Node *) malloc(n_nodes*sizeof(Node));
	for (k=0; k<G.n_nodes; k++)
	{
		G.first[k].visit = 0;
		/*UNIX G.first[k].val=((float)random()/RAND_MAX); */
		G.first[k].val=((float)rand()/RAND_MAX);
		car = &(G.first[k].l_adj);
		/*UNIX n_new=(int) (random()%(Max_out)); */
		n_new=(int) (rand()%(Max_out));
		for (h=0; h<n_new; h++)
		{
			*car = (L_node *) malloc(sizeof(L_node));
			/*UNIX new_edge=(int) random()%(n_nodes-1);*/
			new_edge=(int) rand()%(n_nodes-1);
			new_edge=(new_edge<k)?new_edge:++new_edge;
			(*car)->p_node = G.first + new_edge;
			car = &((*car)->next);
		}
		*car=NULL;
	}
	return(G);
}

void write_file_graph(Graph G,char *fileout)
{
	FILE		*pmf;
	int		k;
	int		m;
	L_node	*car;


	pmf = fopen(fileout,"w");
	fprintf(pmf,"%d\n",G.n_nodes);

	for (k=0; k<G.n_nodes; k++)
	{
		fprintf(pmf,"%f\n",G.first[k].val);
		car = G.first[k].l_adj;

		while (car != NULL)
		{
			m =(int) (car->p_node - G.first);
			fprintf(pmf,",%d",m);
			car = car->next;
		}
		fprintf(pmf,"/\n");
	}

	fclose(pmf);

	return;

} /* end write_file_graph */


/* DGRAPH FUNCTIONS */

/* Global variables for this part of file (other files can't see them) */
static D_node *first_new;		/* The address of the first node of the
											D_graph structure. */
static D_node **first_order;	/* The pointer to the ordered array of
											pointers to the D_graph elements.
											Formally, it is the address of the
											first pointer of the ordered array
											descripted above. */
static Node *first_old;			/* The address of the first node of the
											Graph structure. */


/* Receives as input a graph "Graph G", sorts it and gives back the
	sorted graph with the D_graph structure. */
D_graph build_graph(Graph G)
{
	int i;
	D_graph result;

	/* first_old, first_new and first_order are global for this file and are
		set in this procedure. */

	/* Initializations */
	result.n_nodes=G.n_nodes;
	result.first=(D_node *) malloc(G.n_nodes*sizeof(D_node));
	result.order_array=(D_node **) malloc(G.n_nodes*sizeof(D_node *));
	first_old = G.first;
	first_new = result.first;
	first_order = result.order_array;

	for (i=0;i<G.n_nodes;i++)
	{
		G.first[i].visit=0;
		result.first[i].val=G.first[i].val;
		result.first[i].visit=NULL;
		result.first[i].l_adj_low=NULL;
		result.first[i].l_adj_high=NULL;
		result.first[i].order_link=(D_node **) first_order+i;
		/* also: result.first[i].order_link=&(result.order_array[i]); */
		result.order_array[i]=(D_node *) first_new+i;
		/* also: result.order_array[i]=&(result.first[i]); */
	}

	quick_sort(result.order_array,0,result.n_nodes-1);

	/*for(int l=0;l<result.n_nodes;l++){
		printf("\n%f\n",(result.order_array[l])->val);
	}
	system("pause");*/
	for (i=0;i<G.n_nodes;i++)
		if (G.first[i].visit == 0){
			visit_and_build(&(G.first[i]));
			}
	for (i=0;i<G.n_nodes;i++)
		result.first[i].visit=NULL;
				//	printf("\nSalut %d");

	/*for(int l=0;l<result.n_nodes;l++){
		printf("\n%f\n",result.first[l].val);
	}
	system("pause");*/

	return(result);
}

/* It is a recursive procedure. It receives as input a pointer p to a Node;
	first it marks the Node as visited; then, for each edge E which exits from
	the node, builds the correspondent edge E' in the new D_graph structure and
	reapplies visit_and_build to the other node of E */
void visit_and_build(Node *p)
{
	L_node	*cursor;
	D_node	*old_D_node;	/* Address of the correspondent D_node of *p */
	D_node	*new_D_node;	/* Address of the correspondent D_node of the other
										node of the edge we are visiting */
	Edge		*temp; 			/* Address of the egde we are building (E');
										it's only a support variable. */
	int		old_index,new_index,old_index_order,new_index_order;

	/* first_old, first_new and first_order are global for this file and are
		set in build_graph() */

	p->visit = 1;	/* marked as visit */
	/* Initialization */
	old_index = (int) (p-first_old);
	old_D_node = (D_node *) first_new+old_index;
	old_index_order = (int) (old_D_node->order_link-first_order);
	cursor = p->l_adj;

	/* We follow the adjacence list of *p till the end */
	while (cursor != NULL)
	{
		new_index = (int) (cursor->p_node-first_old);
		new_D_node = (D_node *) first_new+new_index;
		new_index_order = (int) (new_D_node->order_link-first_order);
		if (old_index_order <= new_index_order)
			temp = create_Edge(old_D_node,new_D_node);
		else
			temp = create_Edge(new_D_node,old_D_node);
		if (cursor->p_node->visit == 0)
			visit_and_build(cursor->p_node);
		cursor = cursor->next;
	}
	purge_D_node(old_D_node);
	return;
}

/* inserisce un nuovo L_edge nella lista dei puntatori agli edge.
Per inserire in testa alla lista basta dare il puntatore alla
variabile che punta alla lista e passare NULL come prev */
L_edge *insert_L_edge(L_edge *p,L_edge **p_id,Edge *val)
{
	L_edge *new_L;

	new_L = (L_edge *) malloc(sizeof(L_edge));
	new_L->edge=val;
	new_L->next=*p_id;
	new_L->prev=p;

	if (*p_id != NULL)
		(*p_id)->prev=new_L;
   *p_id = new_L;

	return (new_L);
}

/* inseisce una lista nella lista dei puntatori agli edge.
Per inserire in testa alla lista basta dare il puntatore alla
variabile che punta alla lista e passare NULL come prev  */

void insert_list_in_L_edge_list(L_edge *prev,L_edge **prev_id,L_edge *queue,L_edge *head)
{

	queue->prev=prev;
	head->next=*prev_id;
	if (*prev_id != NULL) (*prev_id)->prev=head;
	*prev_id=queue;

	return;

}

void erase_L_edge(L_edge *dead)
{
	L_edge	**prev;

	if (dead->prev != NULL)
		dead->prev->next=dead->next;
	else
	{	prev = (dead->edge->low.from_l == dead) ?
					&(dead->edge->low.to->l_adj_high) :
						 &(dead->edge->high.to->l_adj_low);
		*prev = dead->next;
	}
	if (dead->next != NULL)
		dead->next->prev=dead->prev;
	free(dead);

	return;
}

Edge *create_Edge(D_node *low,D_node *high)
{
	Edge *new_edge;

	new_edge=(Edge *) malloc(sizeof(Edge));
	new_edge->low.to=low;
	new_edge->high.to=high;
	new_edge->low.from_l=insert_L_edge(NULL,&(low->l_adj_high),new_edge);
	new_edge->high.from_l=insert_L_edge(NULL,&(high->l_adj_low),new_edge);
	return (new_edge);
}

void erase_Edge(Edge *dead)
{
	erase_L_edge(dead->high.from_l);
	erase_L_edge(dead->low.from_l);
	free(dead);
	return;
}

void to_first_L_edge(L_edge *p)
{
	L_edge **first_id;

	if (p != NULL)
	{
		if (p->edge->low.from_l == p)
			first_id = &(p->edge->low.to->l_adj_high);
		else
			first_id = &(p->edge->high.to->l_adj_low);
		if (*first_id!=p)
		{
			p->prev->next = p->next;
			if (p->next != NULL)
				p->next->prev = p->prev;
			p->next = *first_id;
			p->prev = NULL;
			(*first_id)->prev = p;
			*first_id=p;
		}
	}
	return;
}


/* SORT FUNCTIONS  */

void swap(D_node **V,D_node **W)
{
	D_node *tmp;
	(*V)->order_link=W;
	(*W)->order_link=V;
	tmp=*V;
	*V=*W;
	*W=tmp;

	return;
}

int partition(D_node **V,int i,int j,int p)
{
	int right=j,pivot=p;
	int h;	/*counter*/

	swap(V+i,V+pivot);
	pivot=i;
	for (h=i+1;h<=j;h++)
	{
		if (V[pivot]->val > V[pivot+1]->val)
		{
			swap(V+pivot,V+pivot+1);
			pivot++;
		}
		else
		{
			swap(V+pivot+1,V+right);
			right--;
		}
	}

	return(pivot);
}

void quick_sort(D_node **V,int i,int j)
{
	int k;
	if (j-i > QS_REC_DEPTH)
	{
		k=partition(V,i,j,i);
		quick_sort(V,i,k-1);
		quick_sort(V,k+1,j);
	}
	else
		bubble_sort(V,i,j);

	return;
}

void bubble_sort(D_node **V,int i,int j)
{
	int h,l;

	for (h=j-1;h>=i;h--)
		for (l=i;l<=h;l++)
			if (V[l]->val > V[l+1]->val)
				swap(&V[l],&V[l+1]);
	return;
}

/* end of sort functions  */


/* This procedure collapses two D_nodes linked by a Edge (*dead); the lowest
	one (*low) collapses on the highest one (*high) with the value (.val) of
	the lowest.
	good_edges is the list of the edges which can't be erased by other
	delta-star moves (last_good_edge is the end of this list).
	collapse performs both moves of type II (type_III==0) and type III  (type_III==1) of 
	delta star reduction*/
void collapse(L_edge **good_edges,L_edge **last_good_edge,Edge *dead,unsigned char type_III )
{
	D_node  *low, *high;
	L_edge  *cursor, *first, *last;

	low = dead->low.to;
	high = dead->high.to;
	erase_Edge(dead);

	/* For each edge of the l_adj_low of *low, the field .high.to is
		readressed (from *low) to *high; each new D_node linked to *high is
		marked as visited from *high */
	last = first = cursor = low->l_adj_low;
	while (cursor != NULL)
	{
		cursor->edge->low.to->visit=high;
		cursor->edge->high.to=high;
		last = cursor;
		cursor = cursor->next;
	}
	/* Update good_edges list */
	if (first != NULL)
	{
		*last_good_edge = (*last_good_edge == NULL) ? last : *last_good_edge;
		insert_list_in_L_edge_list(NULL,good_edges,first,last);
	}

	/* For each edge of the l_adj_high of *low, the field .low.to is
		readressed (from *low) to *high */
	last = first = cursor = low->l_adj_high;
	while (cursor != NULL)
	{
		if (cursor->edge->high.to != high)
		{
			cursor->edge->low.to=high;
			last = cursor;
			cursor = cursor->next;
		}
		else
		{
			dead = cursor->edge;
			if (cursor==first) first = last = cursor->next;
			cursor = cursor->next;
			erase_Edge(dead);
		}
	}
	/* Update l_adj_high list of *high */
	if (first != NULL)
		insert_list_in_L_edge_list(NULL,&(high->l_adj_high),first,last);

	/* Now we update the value of *high and the links to the ordered array */
	if (type_III)
	{
		high->val=low->val;
		*(high->order_link) = NULL;
		*(low->order_link) = high;
		high->order_link = low->order_link;
		low->order_link = NULL;
	}
	else
	{
	 	*(low->order_link) = NULL;
 		low->order_link = NULL;
	}
	return;
}


/* This procedure erases all duplicate edges of *node */
void purge_D_node(D_node *node)
{
	purge_low_D_node(node);
	purge_hight_D_node(node);
}



/* This procedure erases all duplicate edges in l_adj_low of *node */
void purge_low_D_node(D_node *node)
{
	L_edge *cursor, *temp;

	node->visit = node;
	cursor = node->l_adj_low;
	while (cursor != NULL)
	{
		temp = cursor->next;
		if (cursor->edge->low.to->visit != node)
			cursor->edge->low.to->visit = node;
		else
			erase_Edge(cursor->edge);
		cursor = temp;
	}
}



/* This procedure erases all duplicate edges in l_adj_high of *node */
void purge_hight_D_node(D_node *node)
{
	L_edge *cursor, *temp;

	node->visit = node;
	cursor = node->l_adj_high;
	while (cursor != NULL)
	{
		temp = cursor->next;
		if (cursor->edge->high.to->visit != node)
			cursor->edge->high.to->visit = node;
		else
			erase_Edge(cursor->edge);
		cursor = temp;
	}
}

/* This function performs the move of type I of delta star reduction and returns
	the new edge, if it has been created; NULL otherwise. */
Edge *move_of_type_I(Edge *first,Edge *second)
{
	D_node	*high_node,*med_node;
	Edge		*new_edge;

	high_node = first->high.to;
	med_node = second->high.to;
	erase_Edge(first);
	if (med_node->visit != high_node)
	{
		new_edge = create_Edge(med_node,high_node);
		if (new_edge->low.from_l->next != NULL)
				to_first_L_edge(new_edge->low.from_l->next);
	}
	else
		new_edge = NULL;
	return (new_edge);
}

/* This procedure performs all the operations on the edges lower than the input
	node.
	It is based on a number of assumptions we made in building our algorithm:
	1. Al di sotto del nodo in esame il grafo e' ridotto ("nascosto" nella struttura).
	2. Per ogni nodo piu' basso del nodo in esame, gli archi verso il basso sono
		quelli del grafo ridotto;
	3. Per i nodi al punto 2, degli archi verso l'alto al piu' uno puo' essere
		del grafo ridotto; se esiste e' in testa alla lista di adiacenza.

*/
void low_operation(D_node *base_node)
{
	L_edge 	*good_edges,*last_good_edge;	/* Head and tail of the list of the
															edges of base_node which belongs
															to the reduced graph */
	L_edge 	*cursor,*temp;
	D_node   *actual_node,*third_node;
	Edge   	*actual_edge,*second_edge,*temp2;
	int 	base_index,third_index;

	/* first_order is global for this file and is set in delta_star_reduction()*/

	/* Initialization */
	good_edges = NULL;
	last_good_edge = NULL;
	base_node->visit = base_node->visit;
	base_index = (int) (base_node->order_link-first_order);
	cursor = base_node->l_adj_low;
	while (cursor != NULL)
	{
		actual_edge = cursor->edge;
		if (actual_edge->low.to->visit == base_node)
		{
			/* this edge has already been considered */
			cursor = cursor->next;
			erase_Edge(actual_edge);
		}
		else
		{
			actual_edge->low.to->visit = base_node;
			actual_node = actual_edge->low.to;
			if (actual_node->val == base_node->val)
			{
				/* this is the case of type II move */
				collapse(&good_edges,&last_good_edge,actual_edge,0);
				/*base_index = (int) (base_node->order_link-first_order);*/
				cursor = base_node->l_adj_low;
			}
			else
			{
				second_edge = actual_edge->low.to->l_adj_high->edge;
				third_node = second_edge->high.to;
				third_index = (int) (third_node->order_link-first_order);
				if (third_index < base_index)
				{
					/* this is the case of type I move */
					temp = cursor->next;
					temp2 = move_of_type_I(actual_edge,second_edge);
					cursor = (temp2 != NULL) ? temp2->high.from_l : temp;
				}
				else
				{
					/* the edge is a candidate to belong to the reduced graph */
					cursor = cursor->next;
					to_first_L_edge(actual_edge->low.from_l);
					good_edges = insert_L_edge(NULL,&good_edges,actual_edge);
					last_good_edge = (last_good_edge == NULL) ? good_edges : last_good_edge;
					erase_L_edge(actual_edge->high.from_l);
					actual_edge->high.from_l=good_edges;

				}
			}
		}
	}
	if (good_edges != NULL)
	{
		if (good_edges == last_good_edge)
		{
			/* There is only one candidate; this is the case of type III move */
			/* To use collapse() we must move the edge from good_edges list to
				the adjacence list of base_node. */
			actual_edge = good_edges->edge;
			insert_list_in_L_edge_list(NULL,&(base_node->l_adj_low),good_edges,last_good_edge);
			good_edges = NULL;
			last_good_edge = NULL;
			collapse(&good_edges,&last_good_edge,actual_edge,1);
		}
		if (good_edges != NULL)
			/* At this point no more moves are possible from base_node on the
				lower edges. In good_edges list there are all the edges from
				base_node to its lower nodes which belongs to the reduced graph.
				Thus we move these edges from good_edges list to the adjacence
				list of base_node. */
			insert_list_in_L_edge_list(NULL,&(base_node->l_adj_low),good_edges,last_good_edge);

	}

	return;
}


D_graph delta_star_reduction(Graph G)
{
	D_graph	H;
	int		i;
	D_node	*actual_node;

	H = build_graph(G);
	first_order = H.order_array;
	for (i=1; i<H.n_nodes; i++)
	{
		actual_node = first_order[i];
		low_operation(actual_node);
		purge_hight_D_node(actual_node);
	}
	return (H);
}

/* PGRAPH FUNCTIONS */

	/* P_graph Sort Functions */

void swap_plain(P_node **V,P_node **W)
{
	P_node *tmp;

	tmp=*V;
	*V=*W;
	*W=tmp;

	return;
}

int partition_plain(P_node **V,int i,int j,int p)
{
	int right=j,pivot=p;
	int h;	/*counter*/

	swap_plain(V+i,V+pivot);
	pivot=i;
	for (h=i+1;h<=j;h++)
	{
		if (V[pivot]->val < V[pivot+1]->val)
		{
			swap_plain(V+pivot,V+pivot+1);
			pivot++;
		}
		else
		{
			swap_plain(V+pivot+1,V+right);
			right--;
		}
	}

	return(pivot);
}

void quick_sort_plain(P_node **V,int i,int j)
{
	int k;

	if (j-i > QS_REC_DEPTH)
	{
		k=partition_plain(V,i,j,i);
		quick_sort_plain(V,i,k-1);
		quick_sort_plain(V,k+1,j);
	}
	else
		bubble_sort_plain(V,i,j);

	return;
}

void bubble_sort_plain(P_node **V,int i,int j)
{
	int h,l;

	for (h=j-1;h>=i;h--)
		for (l=i;l<=h;l++)
			if (V[l]->val < V[l+1]->val)
				swap_plain(V+l,V+l+1);

	return;
}

	/* End of sort functions*/

/* circular_flag == 1 a close graph, circular_flag != 1 a open graph */

P_node *build_P_graph_from_array(int dim,SIZE_TYPEVAL *mf_values,int circular_flag)
{
	int			i;
	P_node		*dummy;
	P_node		*car;

	dummy = (P_node *)malloc(sizeof(P_node));
	dummy->next = NULL;
	dummy->val = -MAX_SIZE_TYPEVAL;
	car = dummy;
	for (i=0;i<dim;i++)
	{
		dummy->val = (!circular_flag && mf_values[i]>dummy->val) ? 
								mf_values[i] : dummy->val;
		add_new_plain_node(car,mf_values[i]);
		car = car->next;

	}
	car->next = dummy;
	dummy->val++;
	dummy->prev = car;

	if (circular_flag)	erase_plain_node(dummy);

	return(car);

}


P_node *read_file_P_graph(char *filein)
{
	FILE		*pmf;
	char		key;
	int		circular_flag = 0;
	float		val;
	P_node		*dummy;
	P_node		*car;

	pmf = fopen(filein,"r");

	key=getc(pmf);
	circular_flag = (key == 'c');
	dummy = (P_node *)malloc(sizeof(P_node));
	dummy->next = NULL;
	dummy->val = -MAX_SIZE_TYPEVAL;
	car = dummy;
	while (feof(pmf) == 0)
	{
		fscanf(pmf,"%f",&val);
		dummy->val = (!circular_flag && val>dummy->val) ? val : dummy->val;
		add_new_plain_node(car,val);
		car = car->next;

	}
	fclose(pmf);

	car->next = dummy;
	dummy->val++;
	dummy->prev = car;

	if (circular_flag)	erase_plain_node(dummy);

	return(car);

}

void write_file_P_graph(P_node *Gp,char *fileout)
{
	P_node 		*car;
	FILE		*pmf;

	car = Gp;

	pmf = fopen(fileout,"w");
	if (putc('c',pmf) != EOF)
		do
		{
			fprintf(pmf,"%f\n",car->val);
			car = car->next;
		}while(car != Gp);
	fclose(pmf);

	return;
}

P_node *random_P_graph(int n_nodes)
{

	P_node		*dummy;
	P_node		*car;
	int		i;

	dummy = (P_node *)malloc(sizeof(P_node));
	dummy->next = NULL;
	dummy->val = -MAX_SIZE_TYPEVAL;
	car = dummy;
	for (i=1;i<n_nodes;i++)
	{
		add_new_plain_node(car,((float)rand()/RAND_MAX));
		car = car->next;
		dummy->val = (car->val>dummy->val) ? car->val : dummy->val;
	}

	car->next = dummy;
	dummy->val++;
	dummy->prev = car;

	return(car);
}


void find_min(P_node *P,P_node ***min_array_id,int *n_min_id)
{
	P_node	**min_array;
	int     n_min = 0, i;
	P_list	node_list;
	P_list	**last;
	P_list	*carlist, *temp;
	P_node	*car;

	node_list.node = NULL;
	last = &node_list.next;
	car = P;

	do
	{
		if (car->next->val > car->val && car->prev->val > car->val)
		{
			n_min++;
			*last = (P_list *)malloc (sizeof(P_list));
			(*last)->node = car;
			last = &((*last)->next);
		}

		car = car->next;

	}while(car != P);

	*last = NULL;
	carlist = node_list.next;
	min_array = (P_node **)malloc (n_min*sizeof(P_node *));
	i = 0;
	while (carlist != NULL)
	{
		min_array[i] = carlist->node;
		i++;
		temp = carlist;
		carlist = carlist->next;
		free(temp);
	}

	*min_array_id = min_array;
	*n_min_id = n_min;

	return;

}

P_node *climb_prev(P_node *start)
{
	P_node	*dummy;

	dummy = start->prev;
	while (dummy->prev->val > dummy->val)
	{
		dummy = dummy->prev;
		free(dummy->next);
	}
	return(dummy);
}

P_node *climb_next(P_node *start)
{
	P_node	*dummy;

	dummy = start->next;
	while (dummy->next->val > dummy->val)
	{
		dummy = dummy->next;
		free(dummy->prev);
	}
	return(dummy);
}

void purge_plain_graph(P_node *G)
{
	P_node	*actual;

	actual = G;

	while (actual->next != G)
	{
		if (actual->next->val==actual->val)
			erase_plain_node(actual->next);
		else
			actual = actual->next;
	}

	if (actual->next->val==actual->val && actual->next!=actual)
		erase_plain_node(actual);
	return;
}

void erase_plain_node(P_node *dead)
{
	dead->prev->next = dead->next;
	dead->next->prev = dead->prev;
	free(dead);
	return;
}


void add_new_plain_node(P_node *prev,SIZE_TYPEVAL val)
{
	P_node	*new_node;

	new_node = (P_node *)malloc(sizeof(P_node));
	new_node->prev = prev;
	new_node->next = prev->next;
	new_node->val = val;
	prev->next = new_node;
	if (new_node->next != NULL)
		new_node->next->prev = new_node;

	return;

}


