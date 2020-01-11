/// @file bptree.h
/// @Synopsis B+Tree
/// @author hnu_hefeng@qq.com
/// @version 0.0.1
/// @date 2020-01-11
#ifndef BPTREE_H
#define BPTREE_H

#include <stdbool.h>
#include <stdlib.h>

struct bptree_node_;
typedef struct bptree_node_key_ {
	void *key;
	union {
		void *data;
		struct bptree_node_ *child;
	};
} bptree_node_key;

enum NODE_TYPE {
	NODE_TYPE_NORMAL,
	NODE_TYPE_LEAF
};

enum COMPARE_VALUE {
	BIGGER_THAN,
	LESS_THAN,
	EQUAL_TO,
};

typedef struct bptree_node_ {
	enum NODE_TYPE node_type;
	struct bptree_node_ *parent;
	struct bptree_node_ *prev;
	struct bptree_node_ *next;
	int key_num;
	bptree_node_key *keys;
} bptree_node;

typedef struct bptree_ {
	bptree_node *root;
	int m;
	int (*compare)(const void *left, const void *right);
	void (*destroy)(void *data);
} bptree;

// *******************************************************
/// @Synopsis 初始化tree
///
/// @param: tree
/// @param: m 每个节点最多的孩子个数
/// @param: compare key值比较函数
/// @param: destroy data的销毁函数
// *******************************************************
void bptree_init(bptree *tree, int m, int (*compare)(const void*, const void *), void (*destroy)(void *data));

void bptree_destroy(bptree *tree);

bool bptree_insert(bptree *tree, void *key, void *data);

bool bptree_delete(bptree *tree, void *key);

bool bptree_modify(bptree *tree, void *key, void *data);

void *bptree_search(bptree *tree, bptree_node *node, void *key);

void *bptree_search_range(bptree *tree, void *key_low, void *key_high);

void bptree_dump(bptree *tree);

#endif
