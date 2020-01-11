/// @file main.c
/// @Synopsis
/// @author hnu_hefeng@qq.com
/// @version 0.0.1
/// @date 2020-01-11
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bptree.h"


static void print_tree(bptree *tree)
{
 	bptree_dump(tree); 
 	return;
}

int compare(const void *left, const void *right)
{
	int l = (int64_t)left;
	int r = (int64_t)right;
	if ( l > r )
		return BIGGER_THAN;
	else if ( l < r )
		return LESS_THAN;
	return EQUAL_TO;
}

void destroy(void *data)
{

}

void build_tree(bptree *tree, int m, int (*compare)(const void *, const void *), void (*destroy)(void *data))
{
	bptree_init(tree, m, compare, destroy);	
}

int main(int argc, char **argv)
{
	bptree tree;
	int m = 5;
	build_tree(&tree, m, compare, destroy);
	int i = 20;
	for (;i>0;i--) {
		int64_t key = i;
		int64_t value = key;
		bptree_insert(&tree, (void *)key, (void *)value);
	}
    
	printf("initial tree is :\n");
	print_tree(&tree);
	//search range
	int64_t key_low = 10, key_high=15;
	bptree_search_range(&tree, (void *)key_low, (void *)key_high);

	int64_t k = 11;
	int64_t data = 100;
	//modify
	bptree_modify(&tree, (void *)k, (void *)data); 
	//search
	int64_t v = (int64_t)bptree_search(&tree, tree.root, (void*)k);
	printf("find key %ld=%ld\n", k, v);

	for ( i=20;i>0;i-- ) {
		int64_t key = i;
		bptree_delete(&tree, (void *)key);
		printf("after delete key:%ld\n", key);
		print_tree(&tree);
	}

	bptree_destroy(&tree);
  	return 0;
}
