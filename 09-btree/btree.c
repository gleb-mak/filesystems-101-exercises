#include "solution.h"
#include <stdlib.h>

typedef struct Node Node;

typedef struct btree btree;

struct Node {
	int* keys;
	int cur_keys_num;
	int min_keys_num;
	Node** childs;
	int is_leaf;
};

Node* CreateNode(int min_keys_num, int is_leaf) {
	Node* node = (Node* )malloc(sizeof(Node));
	node->keys = (int* )malloc((2 * min_keys_num - 1) * sizeof(int));
	node->cur_keys_num = 0;
	node->min_keys_num = min_keys_num;
	node->is_leaf = is_leaf;
	node->childs = (Node** )malloc(2 * min_keys_num * sizeof(Node*));
	for (int count = 0; count < 2 * min_keys_num - 1; ++count)
		node->keys[count] = 0;
	for (int count = 0; count < 2 * min_keys_num; ++count)
		node->childs[count] = NULL;
	return node;
}

void FreeNodeWithChilds(Node* root) {
	if (root->is_leaf) {
		free(root->childs);
		free(root->keys);
		free(root);
		return;
	}
	for (int count = 0; count < root->cur_keys_num + 1; ++count) {
		if (root->childs[count] != NULL) {
			FreeNodeWithChilds(root->childs[count]);
		}
	}
	free(root->childs);
	free(root->keys);
	free(root);
}

int IsNodeOrChildsContains(Node* root, int x) {
	for (int count = 0; count < root->cur_keys_num; ++count) {
		if (x == root->keys[count])
			return 1;
		if (x < root->keys[count]) {
			if (root->is_leaf)
				return 0;
			return IsNodeOrChildsContains(root->childs[count], x);
		}
	}
	return 0;
}	

int IsNodeFull(struct Node* node) {
	if (node->cur_keys_num == node->min_keys_num * 2 - 1) 
		return 1;
	return 0;
}

void SplitChild(struct Node* parent, int child_index) {
	int min_keys_num = parent->min_keys_num;
	Node* child = parent->childs[child_index];
	Node* new_node = CreateNode(parent->min_keys_num, child->is_leaf);
	for (int count = min_keys_num; count < 2 * min_keys_num - 1; ++count)
		new_node->keys[count - min_keys_num] = child->keys[count];
	for (int count = min_keys_num; count < min_keys_num * 2; ++count) {
		new_node->childs[count - min_keys_num] = child->childs[count];
		child->childs[count] = NULL;
	}
	new_node->cur_keys_num = min_keys_num - 1;
	child->cur_keys_num = min_keys_num - 1;
	for (int count = parent->cur_keys_num; count > child_index; --count)
		parent->childs[count + 1] = parent->childs[count];
	for (int count = parent->cur_keys_num - 1; count >= child_index; --count)
		parent->keys[count + 1] = parent->keys[count];
	parent->cur_keys_num++;
	parent->childs[child_index + 1] = new_node;
	parent->keys[child_index] = child->keys[min_keys_num - 1];
}

void InsertNotFull(struct Node* root, int x) {
	if (root->is_leaf) {
		for (int count = root->cur_keys_num - 1; count >= 0; --count) {
			if (root->keys[count] <= x) {
				root->keys[count + 1] = x;
				break;
			}
			root->keys[count + 1] = root->keys[count];
		}
		root->cur_keys_num++;
	}
	else {
		for (int count = root->cur_keys_num - 1; count >= 0; --count) {
			if (root->keys[count] <= x) {
				if (IsNodeFull(root->childs[count + 1])) {
					SplitChild(root, count + 1);
					if (root->keys[count + 1] < x) 
						count++;
				}
				InsertNotFull(root->childs[count + 1], x);
				break;
			}
		}
	}
}

void Merge(Node* node, int index) {
	Node* child = node->childs[index];
	Node* brother = node->childs[index + 1];
	int min_keys_num = node->min_keys_num;
	child->keys[min_keys_num - 1] = node->keys[index];
	for (int count = 0; count < brother->cur_keys_num; ++count)
		child->keys[count + min_keys_num] = brother->keys[count];
	if (!child->is_leaf) {
		for(int count = 0; count <= brother->cur_keys_num; ++count)
			child->childs[count + min_keys_num] = brother->childs[count];
	}
	for (int count = index + 1; count < node->cur_keys_num; ++count)
		node->keys[count - 1] = node->keys[count];
	for (int count = index + 2; count <= node->cur_keys_num; ++count)
		node->childs[count - 1] = node->childs[count];
	child->cur_keys_num += brother->cur_keys_num + 1;
	node->cur_keys_num--;
	free(brother->childs);
	free(brother->keys);
	free(brother);
}

void BorrowFromNext(Node* node, int index) {
	Node* child = node->childs[index];
	Node* brother = node->childs[index - 1];
	child->keys[child->cur_keys_num] = node->keys[index];
	if (!child->is_leaf)
		child->childs[child->cur_keys_num + 1] = brother->childs[0];
	node->keys[index] = brother->keys[0];
	for (int count = 1; count < brother->cur_keys_num; ++count)
		brother->keys[count - 1] = brother->keys[count];
	if (!brother->is_leaf) {
		for(int count = 1; count <= brother->cur_keys_num; ++count)
			brother->childs[count - 1] = brother->childs[count];
	}
	child->cur_keys_num++;
	brother->cur_keys_num--;
}

void BorrowFromPrev(Node* node, int index) {
	Node* child = node->childs[index];
	Node* brother = node->childs[index - 1];
	for (int count = child->cur_keys_num - 1; count >= 0; --count)
		child->keys[count + 1] = child->keys[count];
	if (!child->is_leaf) {
		for(int count = child->cur_keys_num; count >= 0; --count)
			child->childs[count + 1] = child->childs[count];
	}
	child->keys[0] = node->keys[index - 1];
	if(!child->is_leaf)
		child->childs[0] = brother->childs[brother->cur_keys_num];
	node->keys[index - 1] = brother->keys[brother->cur_keys_num - 1];
	child->cur_keys_num++;
	brother->cur_keys_num--;
}

void Fill(Node* node, int index) {
	int min_keys_num = node->min_keys_num;
	if (index && node->childs[index - 1]->cur_keys_num >= min_keys_num)
		BorrowFromPrev(node, index);
	else if (index != node->cur_keys_num && node->childs[index + 1]->cur_keys_num >= min_keys_num)
		BorrowFromNext(node, index);
	else {
		if (index != node->cur_keys_num)
			Merge(node, index);
		else
			Merge(node, index - 1);
	}
}

void RemoveFromNode(struct Node* root, int x) {
	if (root == NULL)
		return;
	int min_keys_num = root->min_keys_num;

	if (root->is_leaf) {
		for (int count = 0; count < root->cur_keys_num; ++count) {
			if (root->keys[count] == x) {
				for (int i = count; i < root->cur_keys_num - 1; ++i) {
					root->keys[i] = root->keys[i + 1];
				}
				root->keys[count] = 0;
				root->cur_keys_num--;
				return;
			}
		}
	}
	else {
		for (int count = 0; count < root->cur_keys_num; ++count) {
			if (root->keys[count] == x) {
				if (root->childs[count]->cur_keys_num >= min_keys_num) {
					Node* cur = root->childs[count];
					while (!cur->is_leaf) {
						cur = cur->childs[cur->cur_keys_num];
					}
					int pred = cur->keys[cur->cur_keys_num - 1];
					root->keys[count] = pred;
					RemoveFromNode(root->childs[count], pred);
				}
				else if (root->childs[count + 1]->cur_keys_num >= min_keys_num) {
					Node* cur = root->childs[count + 1];
					while (!cur->is_leaf) 
						cur = cur->childs[0];
					int suc = cur->keys[0];
					root->keys[count] = suc;
					RemoveFromNode(root->childs[count + 1], suc);
				}
				else {
					Merge(root, count);
					RemoveFromNode(root->childs[count], x);
				}
				return;
			}
			else {
//				if (root->is_leaf)
//					return;
				bool flag = (count == root->cur_keys_num) ? true : false;
				if (root->childs[count]->cur_keys_num < min_keys_num)
					Fill(root, count);
				if (flag && count > root->cur_keys_num)
					RemoveFromNode(root->childs[count - 1], x);
				else
					RemoveFromNode(root->childs[count], x);
			}
		}
	}
}

//---------------------------------------------------------------------------

struct btree
{
	Node* root;
	int min_keys_num;
};

struct btree* btree_alloc(unsigned int L)
{
	(void) L;
//	L--;
	btree* new_btree = (btree* )malloc(sizeof(btree));
	new_btree->min_keys_num = L;
	new_btree->root = NULL;
	return new_btree;
}

int btree_is_empty(struct btree *t) {
	if (NULL == t->root)
		return 1;
	return 0;
}

void btree_free(struct btree *t)
{
	(void) t;
	if (!btree_is_empty(t))
		FreeNodeWithChilds(t->root);
	free(t);
}


void btree_insert(struct btree *t, int x)
{
	(void) t;
	(void) x;
	Node* root = t->root;
	int min_keys_num = t->min_keys_num;
	if (btree_is_empty(t)) {
		root = CreateNode(t->min_keys_num, 1);
		root->keys[0] = x;
		root->cur_keys_num = 1;
		t->root = root;
		return;
	}
	if (IsNodeFull(root)) {
		Node* new_root = CreateNode(min_keys_num, 0);
		t->root = new_root;
		new_root->childs[0] = root;
		SplitChild(new_root, 0);
		Node* needed_child = (x > new_root->keys[0]) ? new_root->childs[1] : new_root->childs[0];
		InsertNotFull(needed_child, x);
	}
	else
		InsertNotFull(root, x);
}

void btree_delete(struct btree *t, int x)
{
	(void) t;
	(void) x;
	if (btree_is_empty(t))
		return;
	RemoveFromNode(t->root, x);
	if (t->root->cur_keys_num == 0) {
		Node* copy = t->root;
		if (copy->is_leaf)
			t->root = NULL;
		else
			t->root = (t->root)->childs[0];
		free(copy);
	}
}	

bool btree_contains(struct btree *t, int x)
{
	(void) t;
	(void) x;
	if (btree_is_empty(t))
		return false;
	return IsNodeOrChildsContains(t->root, x);
/*	if (IsNodeOrChildsContains(t->root, x))
		return true;
	return false;*/
}

struct btree_iter {
	btree* tree;
	Node** path;
	int cur_depth;
	int* path_indexes;
};

struct btree_iter* btree_iter_start(struct btree *t)
{
	(void) t;
	struct btree_iter* iter = (struct btree_iter* )malloc(sizeof(struct btree_iter));
	iter->tree = t;
	if (btree_is_empty(t)) {
		iter->path = NULL;
		iter->cur_depth = 0;
		iter->path_indexes = NULL;
		return iter;
	}
	int depth = 0;
	Node* node = t->root;
	while (!node->is_leaf) {
		node = node->childs[0];
		depth++;
	}
	iter->cur_depth = depth;
	iter->path = (Node** )malloc(depth * sizeof(Node*));
	iter->path_indexes = (int*)malloc(depth * sizeof(int));
	node = t->root;
	int count = 0;
	while (!node->is_leaf) {
		iter->path[count] = node;
		count++;
        node = node->childs[0];
		iter->path_indexes[count] = 0;
    }
	return iter;
}

void btree_iter_end(struct btree_iter *i)
{
	(void) i;
	free(i->path);
	free(i->path_indexes);
	free(i);
}

bool btree_iter_next(struct btree_iter *i, int *x)
{
	(void) i;
	(void) x;
	int cur_depths = i->cur_depth;
	Node* cur_node = i->path[cur_depths];
	int idx = i->path_indexes[cur_depths];
	if (i->path_indexes[cur_depths] < cur_node->cur_keys_num - 1) {
		if (cur_node->is_leaf) {
			*x = cur_node->keys[i->path_indexes[cur_depths] + 1];
			i->path_indexes[cur_depths]++;
		}
		else {
			*x = cur_node->childs[idx + 1]->keys[0];
			i->path[idx + 1] = cur_node->childs[idx + 1];
			i->path_indexes[cur_depths]++;
			i->path_indexes[cur_depths + 1] = 0;
			i->cur_depth++;
		}
	}
	else {
		//if (cur_node->is_leaf) {
			//Node* tmp = i->path[cur_depth - 1];
			//while ()
			*x = i->path[i->cur_depth - 1]->keys[i->path_indexes[cur_depths - 1] + 1];
			i->path[idx - 1] = i->path[i->cur_depth - 1];
			i->path_indexes[cur_depths - 1]++;
			i->cur_depth++;
		//}
	/*	else {
			cur_node = cur_node->childs[i->path_indexes[cur_depths]]
		}*/
	}
	return false;
}
