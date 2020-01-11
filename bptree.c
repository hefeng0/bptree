/// @file bptree.c
/// @Synopsis 
/// @author hnu_hefeng@qq.com
/// @version 0.0.1
/// @date 2020-01-11
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "bptree.h"

void destroy_node(bptree *tree, bptree_node *node);
void split(bptree *tree, bptree_node *node);
void merge(bptree *tree, bptree_node *node);
void travel(bptree *tree, bptree_node *node, int depth, \
	void (*do_something)(bptree *tree, bptree_node *));
bptree_node *bptree_create_node(bptree *tree, bptree_node *parent);
bptree_node_key *search_leaf_node(bptree *tree, bptree_node *node, void *key, int rw);

enum {
	BELOW_FACTOR,
	MATCH_FACTOR,
	ABOVE_FACTOR,
};

// *******************************************************
/// @Synopsis 查找key值应该洛在哪个叶子节点
///
/// @param: tree
/// @param: key 待查找的key值
/// @param: rw 查找为0，insert为1
///
/// @returns: 返回这个叶子节点在父节点中对应的bptree_node_key结构
// *******************************************************
#define search_node(tree, key, rw)	 \
	bptree_node_key *leaf_key = NULL; \
	do \
	{ \
		if ( tree->root->node_type == NODE_TYPE_LEAF ) { \
			bptree_node_key dum = {0, tree->root}; \
			leaf_key = &dum; \
		} else { \
			leaf_key = search_leaf_node(tree, tree->root, key, rw); \
		} \
	} while(0)


void bptree_init(bptree *tree, int m, int (*compare)(const void *, const void *), void (*destroy)(void *))
{
	tree->m = m;
	tree->compare = compare;
	tree->destroy = destroy;
	tree->root = bptree_create_node(tree, NULL);
	tree->root->node_type = NODE_TYPE_LEAF;
}

bool node_is_root(bptree_node *node)
{
	return node->parent == NULL;
}

bool node_is_full(bptree *tree, bptree_node *node)
{
	return tree->m <= node->key_num;
}

int node_fill_factor(bptree *tree, bptree_node *node)
{
	if ( node->key_num < tree->m/2 )
		return BELOW_FACTOR;
	else if ( node->key_num == tree->m/2 )
		return MATCH_FACTOR;
	return ABOVE_FACTOR;
}

/*
 * 从指定节点中二分查找key的位置
 * search_type=0:精确查找key的位置
 * search_type=1:查找最后一个不大于key值的位置
 * rw=0:读
 * rw=1:写，这时候要顺序更新非叶子节点的key值
 * 1   3   5
 * 1 2 3 4 5 6
 */
int search_key(bptree *tree, bptree_node *node, void *key, int search_type, int rw) 
{
	bptree_node_key *keys=node->keys;
	int key_num = node->key_num;
	int node_type = node->node_type;
	int beg = 0, end = key_num-1;
	int mid = search_type==1? 0:-1;
	int last_mid = mid;
	while (beg<=end) {
		//忽略mid值被删除的情况，可以考虑标记删除
		last_mid = mid;
		mid = (beg+end)/2;
		//两个元素的情况，可能会死循环
		if ( mid == last_mid ) {
			int rtn = tree->compare(key, keys[end].key);
			if ( rtn == EQUAL_TO )
				return end;
			if ( rtn != LESS_THAN )
				mid = end;
			break;
		}
		int rtn = tree->compare(key, keys[mid].key);
		switch (rtn) {
		case BIGGER_THAN:
			beg = mid;
			break;
		case LESS_THAN:
			end = mid;
			break;
		case EQUAL_TO:
			return mid;
		default:
			break;
		}
	}
	if ( (node_type == NODE_TYPE_NORMAL) && rw && mid <= 0 )
		keys[mid].key = key;
	
	return search_type==1?mid:-1;
}

void *bptree_search(bptree *tree, bptree_node *node, void *key)
{
	int search_type = node->node_type==NODE_TYPE_NORMAL?1:0;
	int idx_key = search_key(tree, node, key, search_type, 0);
	enum NODE_TYPE node_type = node->node_type;
	if ( node_type == NODE_TYPE_LEAF ) {
		if ( tree->compare(key, node->keys[idx_key].key) == EQUAL_TO )
			return node->keys[idx_key].data;
	}
	return bptree_search(tree, node->keys[idx_key].child, key);
}

void *bptree_search_range(bptree *tree, void *key_low, void *key_high)
{
	search_node(tree, key_low, 0);
	int rw = 0;
	int search_type = 1;

	int idx_key = search_key(tree, leaf_key->child, key_low, search_type, rw);
	bptree_node *node = leaf_key->child;
	int i = idx_key;
	bool found = false;
	while(node) {
		bool finish = false;
		for ( ;i<node->key_num;i++ ) {
			if ( !found && tree->compare(key_low, node->keys[i].key)==BIGGER_THAN )
				continue;
			found = true;
			int rtn = tree->compare(key_high, node->keys[i].key);
			if ( rtn == LESS_THAN ) {
				finish = true;
				break;
			}
			printf("key=%ld data=%ld\n", (int64_t)node->keys[i].key, (int64_t)node->keys[i].data);
		}
		if ( finish )
			break;
		i = 0;
		node = node->next;
	}
}

//查找key应该落在哪个叶子节点
bptree_node_key *search_leaf_node(bptree *tree, bptree_node *node, void *key, int rw)
{
	int search_type = node->node_type==NODE_TYPE_NORMAL?1:0;
	int idx_key = search_key(tree, node, key, search_type, rw);
	int child_node_type = node->keys[idx_key].child->node_type;
	if ( child_node_type == NODE_TYPE_NORMAL )
		return search_leaf_node(tree, node->keys[idx_key].child, key, rw);
	else if ( child_node_type == NODE_TYPE_LEAF )
		return node->keys+idx_key;
		
	//不可能到这里
	return NULL;
}

// *******************************************************
/// @Synopsis 将key插入节点的keys中，可能是叶子节点也可能不是
///
/// @param: tree
/// @param: node
/// @param: node_type
/// @param: key
/// @param: child_or_data
// *******************************************************
void insert_key(bptree *tree, bptree_node *node, int node_type, void *key, void *child_or_data)
{
	int i = node->key_num-1;
	for ( ;i>=0;i-- ) {
		int rtn = tree->compare(key, node->keys[i].key);
		if ( rtn != LESS_THAN )
			break;
		node->keys[i+1] = node->keys[i];
	}
	node->keys[i+1].key = key;
	if ( node_type == NODE_TYPE_NORMAL ) {
		bptree_node *child = (bptree_node *)child_or_data;
		child->parent = node;
		node->keys[i+1].child = child;
	} else if ( node_type == NODE_TYPE_LEAF ) {
		node->keys[i+1].data = child_or_data;
	}
	node->key_num++;
}

// *******************************************************
/// @Synopsis 如果待插入的key值小于节点keys[0].key，需要递归更新
///
/// @param: tree
/// @param: node
/// @param: key_old
/// @param: key
// *******************************************************
void modify_key(bptree *tree, bptree_node *node, void *key_old, void *key)
{
	if ( !node )
		return;
	int i=0;
	for ( ;i<node->key_num;i++ ) {
		if ( tree->compare(key_old, node->keys[i].key) != EQUAL_TO )
			continue;
		node->keys[i].key = key;
		break;
	}	
	if ( i==0 )
		modify_key(tree, node->parent, key_old, key); 
}

void delete_key(bptree *tree, bptree_node *node, int idx_key)
{
	int i = idx_key+1;
	for ( ;i<node->key_num;i++ ) {
		node->keys[i-1] = node->keys[i];	
	}
	node->key_num--;
}

// *******************************************************
/// @Synopsis 在node后面插入一个node，并返回这个node指针
///
/// @param: tree
/// @param: node
///
/// @returns: 
// *******************************************************
bptree_node *insert_node_after(bptree *tree, bptree_node *node)
{
	bptree_node *new_sibling_node = bptree_create_node(tree, node->parent);
	new_sibling_node->node_type = node->node_type;
	new_sibling_node->next = node->next;
	new_sibling_node->prev = node;
	if ( new_sibling_node->next )
		new_sibling_node->next->prev = new_sibling_node;
	node->next = new_sibling_node;
	return new_sibling_node;
}

//节点已经没有子结点，从兄弟链表和父节点的keys中删除
void delete_node(bptree *tree, bptree_node *node, void *parent_key)
{
	if ( node->key_num > 0 ) {
		//还有子结点，先不删除
		return;
	}
	//从兄弟链表中删除
	if ( node->prev ) {
		bptree_node *prev = node->prev;
		prev->next = node->next;
	}
	if ( node->next ) {
		bptree_node *next = node->next;
		next->prev = node->prev;
	}
	//从父节点的key中删除
	int idx_key = search_key(tree, node->parent, parent_key, 1, 0);
	if ( idx_key >= 0 ) {
		delete_key(tree, node->parent, idx_key);
	}
	if ( idx_key == 0 )
		modify_key(tree, node->parent, parent_key, node->parent->keys[0].key);
	destroy_node(tree, node);		
}

// *******************************************************
/// @Synopsis 将数据插入叶子节点，如果满了递归分裂
///
/// @param: tree
/// @param: leaf_key
/// @param: key
/// @param: data
///
/// @returns: 
// *******************************************************
bool insert_into_leaf(bptree *tree, bptree_node_key *leaf_key, void *key, void *data)
{
	bptree_node *leaf = leaf_key->child;
	//插入数据
	if ( leaf->key_num <= 0 ) {
		leaf->keys[0].key = key;
		leaf->keys[0].data = data;
		leaf->key_num++;
	} else {
		insert_key(tree, leaf, NODE_TYPE_LEAF, key, data);
	}

	//最后检查，如果满了，就分裂
	split(tree, leaf);
	return true;
}

// *******************************************************
/// @Synopsis 根据查找到的叶子节点所在的bptree_node_key指针
/// 	      进行插入操作
///
/// @param: tree
/// @param: leaf_key
/// @param: key
/// @param: data
///
/// @returns: 
// *******************************************************
bool insert_with_node_key(bptree *tree, bptree_node_key *leaf_key, void *key, void *data)
{
	int search_type = 0;
	int idx_key = search_key(tree, leaf_key->child, key, search_type, 1);
	if ( idx_key != -1 )
		return false;
	insert_into_leaf(tree, leaf_key, key, data);
	return true;
}

bool bptree_insert(bptree *tree, void *key, void *data)
{
	//这里还不能调用search_node，返回值leaf_key依赖里面的局部变量
	search_node(tree, key, 1);
	return insert_with_node_key(tree, leaf_key, key, data);
}

bool bptree_modify(bptree *tree, void *key, void *data)
{
	search_node(tree, key, 0);
	int search_type = 0;
	int idx_key = search_key(tree, leaf_key->child, key, search_type, 0);
	if ( idx_key == -1 )
		return false;
	leaf_key->child->keys[idx_key].data = data;
	return true;
}

// *******************************************************
/// @Synopsis 删除key，如果删除后节点的fill factor BELOW_FACTOR，
///	      那么还得跟相邻兄弟节点合并
///
/// @param: tree
/// @param: key
///
/// @returns: 
// *******************************************************
bool bptree_delete(bptree *tree, void *key)
{
	search_node(tree, key, 0);
	int search_type = 0;
	int idx_key = search_key(tree, leaf_key->child, key, search_type, 0);
	if ( idx_key == -1 )
		return false;
	//删除key
	delete_key(tree, leaf_key->child, idx_key);
	//递归向上更新key
	if ( idx_key == 0 && leaf_key->child->key_num>0 )
		modify_key(tree, leaf_key->child->parent, key, \
			leaf_key->child->keys[0].key);
	merge(tree, leaf_key->child);
	//如果根节点只有一个key，并且不是leaf节点，那么可以直接把根节点删除
	if ( tree->root->key_num ==1 && tree->root->node_type == NODE_TYPE_NORMAL ) {
		bptree_node *root = tree->root;
		tree->root = tree->root->keys[0].child;
		tree->root->parent = NULL;
		root->key_num = 0;
		destroy_node(tree, root);
	}
	return true;
}

//将right节点合并到left节点
//有两种情况：1，left和right是兄弟节点；2，不是兄弟节点
void merge_sibling_node(bptree *tree, bptree_node *left, bptree_node *right)
{
	int copy_to = left->key_num;
	int num_copy = right->key_num;
	memcpy(left->keys+copy_to, right->keys, sizeof(bptree_node_key)*num_copy);
	if ( left->node_type == NODE_TYPE_NORMAL ) {
		for ( int i=copy_to;i<copy_to+num_copy;i++ ) { 
			left->keys[i].child->parent = left;
		}
	}
	left->key_num += right->key_num;
	right->key_num = 0;
	void *parent_key = right->keys[0].key;
	delete_node(tree, right, parent_key);
}

// *******************************************************
/// @Synopsis 将node节点和兄弟节点合并，这里情况比较复杂，
///	      要看兄弟节点的fill factor以及兄弟节点是否是
///	      真的兄弟，也可能不是同一个parent
///
/// @param: tree
/// @param: node
// *******************************************************
void merge(bptree *tree, bptree_node *node)
{
	if ( !node )
		return;
	bptree_node *parent = node->parent;
	int fill_factor = node_fill_factor(tree, node);
	if ( fill_factor != BELOW_FACTOR )
		return;
	if ( node->prev && node_fill_factor(tree, node->prev) == ABOVE_FACTOR ) {
		//左兄弟有足够key，可以拷贝最后一个过来，node->prev和node可以不是兄弟节点
		bptree_node_key *key = &node->prev->keys[node->prev->key_num-1];
		insert_key(tree, node, node->node_type, key->key, key->data);
		node->prev->key_num--;
		modify_key(tree, node->parent, node->keys[1].key, node->keys[0].key);
	} else if ( node->next && node_fill_factor(tree, node->next) == ABOVE_FACTOR ) {
		//右兄弟有足够key，可以拷贝第一个过来，node->next和node可以不是兄弟节点
		bptree_node_key *key = &node->next->keys[0];
		void *key_old = key->key;
		insert_key(tree, node, node->node_type, key->key, key->data);
		delete_key(tree, node->next, 0);
		modify_key(tree, node->next->parent, key_old, node->next->keys[0].key);
	} else if ( node->prev ) {
		//与左节点合并
		if ( node->prev->parent != node->parent )
			merge_sibling_node(tree, node->prev->parent, node->parent);
		merge_sibling_node(tree, node->prev, node);
		split(tree, node->prev);
	} else if ( node->next ) {
		//与右节点合并
		if ( node->parent != node->next->parent )
			merge_sibling_node(tree, node->parent, node->next->parent);
		merge_sibling_node(tree, node, node->next);
		split(tree, node);
	} else {
		//没有左右节点，应该忽略
	}

	merge(tree, parent);
}

// *******************************************************
/// @Synopsis 为了方便，采用节点分裂，插入parent节点的方式
///	      如果parent节点也满，那么需要递归分裂
///
/// @param: tree
/// @param: node
// *******************************************************
void split(bptree *tree, bptree_node *node)
{
	if ( node == NULL )
		return;
	if ( !node_is_full(tree, node) )
		return;
	//向父节点出入一个key
	if ( node_is_root(node) ) {
		bptree_node *root = bptree_create_node(tree, NULL);
		insert_key(tree, root, NODE_TYPE_NORMAL, \
			node->keys[0].key, (void *)node);
		tree->root = root;
	}
	bptree_node *new_sibling_node = insert_node_after(tree, node);
	//拷贝数据
	int copy_from = node->key_num/2;
	int num_copy = node->key_num - copy_from;
	memcpy(new_sibling_node->keys, node->keys+copy_from, \
		sizeof(bptree_node_key)*num_copy);
	new_sibling_node->key_num = num_copy;
	node->key_num = node->key_num/2;
	insert_key(tree, node->parent, NODE_TYPE_NORMAL, \
		new_sibling_node->keys[0].key, (void *)new_sibling_node);
	if ( new_sibling_node->node_type == NODE_TYPE_NORMAL ) {
		//修改子结点的parent指针
		for ( int i=0;i<new_sibling_node->key_num;i++ )
			new_sibling_node->keys[i].child->parent = new_sibling_node;
	}
	return split(tree, node->parent);
}

bptree_node *bptree_create_node(bptree *tree, bptree_node *parent) 
{
	bptree_node *node = NULL;
	node = (bptree_node *)malloc(sizeof(bptree_node));
	assert(node!=NULL);
	node->keys = (bptree_node_key *)malloc(sizeof(bptree_node_key) * tree->m);
	assert(node->keys);
	int i=0;
	for ( i=0;i<tree->m;i++ ) {
		node->keys[i].child = NULL;
	}
	node->key_num = 0;
	node->parent = parent;
	node->prev = NULL;
	node->next = NULL;

	return node;
}

void destroy_node(bptree *tree, bptree_node *node)
{
	assert(tree!=NULL);
	assert(node!=NULL);
	if (node->node_type == NODE_TYPE_LEAF && tree->destroy ) {
		for ( int i=0;i<node->key_num;i++ ) {
			tree->destroy(node->keys[i].data);
		}
	}
	free(node->keys);
	free(node);
}

void bptree_destroy(bptree *tree)
{
	travel(tree, tree->root, 0, destroy_node);
}

// *******************************************************
/// @Synopsis 为了跟destroy_node函数参数一致，引入了tree参数
///	      其实用不着
///
/// @param: tree
/// @param: node
// *******************************************************
void print_node(bptree *tree, bptree_node *node)
{
	printf("node %lu next %lu prev %lu parent %lu\t", \
		(unsigned long)node, (unsigned long)node->next, \
		(unsigned long)node->prev, (unsigned long)node->parent);
	for (int i=0;i<node->key_num;i++) {
		printf("%ld\t", (int64_t)node->keys[i].key);
	}
	printf("\n");
}

void travel(bptree *tree, bptree_node *node, int depth, void (*do_something)(bptree *tree, bptree_node *))
{
	do_something(tree, node);
	if ( node->node_type == NODE_TYPE_LEAF )
		return;
	for (int i=0;i<node->key_num;i++) {
		travel(tree, node->keys[i].child, depth+1, do_something);
	}
}

void bptree_dump(bptree *tree)
{
	travel(tree, tree->root, 0, print_node);
	printf("\n");
}
