#ifndef DELTAGRP_H
#define DELTAGRP_H

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

/* If you change the typeval, you have to change also MAX_SIZE_TYPEVAL */
#ifndef MAX_SIZE_TYPEVAL
	typedef float SIZE_TYPEVAL;
	#define MAX_SIZE_TYPEVAL FLT_MAX
#endif

#define QS_REC_DEPTH 7000

/* GRAPH STRUCTURES */

struct Node{
	struct L_node	*l_adj;
	SIZE_TYPEVAL	val;
	int	visit;
};

struct L_node{
	struct Node		*p_node;
	struct L_node	*next;
};

struct Graph{
	struct Node	*first;
	int		n_nodes;
};

typedef struct Node Node;
typedef struct L_node L_node;
typedef struct Graph Graph;

/* DGRAPH STRUCTURES */

struct Edge_links{
	struct D_node	*to;
	struct L_edge	*from_l;
};

struct Edge{
	struct Edge_links	low;
	struct Edge_links	high;
};

struct L_edge{
	struct L_edge	*next;
	struct L_edge	*prev;
	struct Edge		*edge;
};

struct D_node{
	struct L_edge	*l_adj_low;
	struct L_edge	*l_adj_high;
	SIZE_TYPEVAL			val;
	struct D_node	*visit;
	struct D_node	**order_link;
};

struct D_graph{
	struct D_node	*first;
	int				n_nodes;
	struct D_node	**order_array;
};

typedef struct Edge_links Edge_links;
typedef struct Edge Edge;
typedef struct L_edge L_edge;
typedef struct D_node D_node;
typedef struct D_graph D_graph;

/* PGRAPH STRUCTURES */

struct P_node{
	struct P_node	*next;
	struct P_node	*prev;
	SIZE_TYPEVAL	val;
};

struct P_graph{
	struct P_node	*first;
	int		n_nodes;
};

struct P_list{
	struct P_node	*node;
	struct P_list	*next;
};

typedef struct P_node P_node;
typedef struct P_graph P_graph;
typedef struct P_list P_list;


/* GRAPH PROTOTYPE FUNCTIONS */
Graph new_graph(int);
void add_new_edge_graph(Node *,Node *);
void erase_edge_graph(L_node **);
void insert_new_edge_graph(Node *,L_node **);
void insert_edge_graph(L_node *,L_node **);
void purge_graph(Graph);
void destroy_graph(Graph);
Graph read_file_graph(char *);
Graph random_graph(int,int);
void write_file_graph(Graph,char *);


/* DGRAPH PROTOTYPE FUNCTIONS */
D_graph build_graph(Graph);
void visit_and_build(Node *);
L_edge *insert_L_edge(L_edge *,L_edge **,Edge *);
void insert_list_in_L_edge_list(L_edge *,L_edge **,L_edge *,L_edge *);
void erase_L_edge(L_edge *);
Edge *create_Edge(D_node *,D_node *);
void erase_Edge(Edge *);
void to_first_L_edge(L_edge *);
void swap(D_node **,D_node **);
int partition(D_node **,int,int,int);
void quick_sort(D_node **,int,int);
void bubble_sort(D_node **,int,int);
void collapse (L_edge **,L_edge **,Edge *,unsigned char);
void purge_D_node(D_node *);
void purge_low_D_node(D_node *);
void purge_hight_D_node(D_node *);
Edge *move_of_type_I(Edge *,Edge *);
void low_operation(D_node *);
D_graph delta_star_reduction(Graph);


/* PGRAPH PROTOTYPE FUNCTIONS */
void find_min(P_node *,P_node ***,int *);
void swap_plain(P_node **,P_node **);
int partition_plain(P_node **,int,int,int);
void quick_sort_plain(P_node **,int,int);
void bubble_sort_plain(P_node **,int,int);
P_node *climb_prev(P_node *);
P_node *climb_next(P_node *);
void purge_plain_graph(P_node *);
void erase_plain_node(P_node *);
void add_new_plain_node(P_node *,SIZE_TYPEVAL);
P_node *random_P_graph(int);
P_node *read_file_P_graph(char *);
void write_file_P_graph(P_node *,char *);
P_node *build_P_graph_from_array(int,SIZE_TYPEVAL *,int);

#endif