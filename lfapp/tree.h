/*
 * $Id$
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LFL_LFAPP_TREE_H__
#define __LFL_LFAPP_TREE_H__

namespace LFL {

struct RedBlackTreeZipper { RedBlackTreeZipper(bool update=0) {} };

template <class K, class V, class Zipper = RedBlackTreeZipper> struct RedBlackTreeNode {
    enum { Left, Right };
    typedef RedBlackTreeNode<K,V,Zipper> Self;
    K key; unsigned val:31, left, right, parent, color:1;
    RedBlackTreeNode(K k, unsigned v, unsigned P, bool C=0) : key(k), val(v), left(0), right(0), parent(P), color(C) {}
    void SwapKV(RedBlackTreeNode *n) { swap(key, n->key); unsigned v=val; val=n->val; n->val=v; }
    virtual K GetKey(Zipper*) const { return key; }
    virtual int WalkRight  (Zipper *z) const { return right; }
    virtual int UnwalkRight(Zipper *z) const { return parent; }
    virtual bool LessThan(const K &k, int, Zipper*) const { return key < k; }
    virtual bool MoreThan(const K &k, int, Zipper*) const { return k < key; }
    virtual void ComputeStateFromChildren(const Self*, const Self*) {}
};

template <class K, class V, class Zipper = RedBlackTreeZipper, class Node = RedBlackTreeNode<K,V,Zipper> >
struct RedBlackTree {
    enum Color { Red, Black };
    typedef RedBlackTree<K,V,Zipper,Node> Self;
    struct Iterator { 
        Self *tree; int ind; K key; V *val; Zipper zipper;
        Iterator(Self *T=0, int I=0)        : tree(T), ind(I), key(0), val(0) {                  if (ind) LoadKV(); }
        Iterator(Self *T, int I, Zipper &z) : tree(T), ind(I), key(0), val(0) { swap(z, zipper); if (ind) LoadKV(); }
        Iterator& operator--() { if ((ind = tree->DecrementNode(ind, &zipper))) LoadKV(); else val = 0; return *this; }
        Iterator& operator++() { if ((ind = tree->IncrementNode(ind, &zipper))) LoadKV(); else val = 0; return *this; }
        bool operator!=(const Iterator &i) { return tree != i.tree || ind != i.ind; }
        void LoadKV() { if (const Node *n = &tree->node[ind-1]) { key = n->GetKey(&zipper); val = &tree->val[n->val]; } }
    };
    struct ConstIterator { 
        const Self *tree; int ind; K key; const V *val; Zipper zipper;
        ConstIterator(const Self *T=0, int I=0)        : tree(T), ind(I), key(0), val(0) {                  if (ind) LoadKV(); }
        ConstIterator(const Self *T, int I, Zipper &z) : tree(T), ind(I), key(0), val(0) { swap(z, zipper); if (ind) LoadKV(); }
        ConstIterator(const Iterator &i) : tree(i.tree), ind(i.ind), key(i.key), val(i.val), zipper(i.zipper) {}
        ConstIterator& operator-- () { if ((ind = tree->DecrementNode(ind, &zipper))) LoadKV(); else val = 0; return *this; }
        ConstIterator& operator++ () { if ((ind = tree->IncrementNode(ind, &zipper))) LoadKV(); else val = 0; return *this; }
        bool operator!=(const ConstIterator &i) { return tree != i.tree || ind != i.ind; }
        void LoadKV() { if (const Node *n = &tree->node[ind-1]) { key = n->GetKey(&zipper); val = &tree->val[n->val]; } }
    };
    struct Query {
        const K key; const V *val; Zipper z;
        Query(const K &k, const V *v=0, bool update=0) : key(k), val(v), z(update) {}
    };

    FreeListVector<Node> node;
    FreeListVector<V> val;
    bool update_on_dup_insert=1;
    int head=0, count=0;

    int size() const { return count; }
    void Clear() { head=count=0; node.Clear(); val.Clear(); }
    Iterator  Begin() { Zipper z; return Iterator(this, head ? GetMinNode(head)     : 0, z); }
    Iterator RBegin() { Zipper z; return Iterator(this, head ? GetMaxNode(head, &z) : 0, z); }

    ConstIterator Find      (const K &k) const   { Query q=GetQuery(k);        return Iterator(this, FindNode      (&q), q.z); }
    /**/ Iterator Find      (const K &k)         { Query q=GetQuery(k);        return Iterator(this, FindNode      (&q), q.z); }
    ConstIterator LowerBound(const K &k) const   { Query q=GetQuery(k);        return Iterator(this, LowerBoundNode(&q), q.z); }
    /**/ Iterator LowerBound(const K &k)         { Query q=GetQuery(k);        return Iterator(this, LowerBoundNode(&q), q.z); }
    /**/ Iterator Insert(const K &k, const V &v) { Query q=GetQuery(k, &v, 1); return Iterator(this, InsertNode    (&q), q.z); }
    bool          Erase (const K &k)             { Query q=GetQuery(k, 0,  1); return EraseNode(&q); }

    virtual Query GetQuery(const K &k, const V *v=0, bool update=0) const { return Query(k, v); }
    virtual K GetCreateNodeKey(const Query *q) const { return q->key; }
    virtual void ComputeStateFromChildrenOnPath(Query *q) {}
    virtual int ResolveInsertCollision(int ind, Query *q) { 
        Node *n = &node[ind-1];
        if (update_on_dup_insert) val[n->val] = *q->val;
        return ind;
    }

    int FindNode(Query *q) const {
        const Node *n;
        for (int ind=head; ind;) {
            n = &node[ind-1];
            if      (n->MoreThan(q->key, ind, &q->z)) ind = n->left;
            else if (n->LessThan(q->key, ind, &q->z)) ind = n->right;
            else return ind;
        } return 0;
    }
    int LowerBoundNode(Query *q) const {
        const Node *n = 0;
        int ind = head;
        while (ind) {
            n = &node[ind-1];
            if      (n->MoreThan(q->key, ind, &q->z)) { if (!n->left) break; ind = n->left; }
            else if (n->LessThan(q->key, ind, &q->z)) {
                if (!n->right) { ind = IncrementNode(ind, &q->z); break; }
                ind = n->right;
            } else break;
        }
        return ind;
    }
    int IncrementNode(int ind, Zipper *z=0) const {
        const Node *n = &node[ind-1], *p;
        if (n->right) for (n = &node[(ind = n->WalkRight(z))-1]; n->left; n = &node[(ind = n->left)-1]) {}
        else {
            int p_ind = GetParent(ind);
            while (p_ind && ind == (p = &node[p_ind-1])->right) { ind=p_ind; p_ind=p->UnwalkRight(z); }
            ind = p_ind;
        }
        return ind;
    }
    int DecrementNode(int ind, Zipper *z=0) const {
        const Node *n = &node[ind-1], *p;
        if (n->left) for (n = &node[(ind = n->left)-1]; n->right; n = &node[(ind = n->WalkRight(z))-1]) {}
        else {
            int p_ind = GetParent(ind);
            while (p_ind && ind == (p = &node[p_ind-1])->left) { ind=p_ind; p_ind=p->parent; }
            if ((ind = p_ind)) p->UnwalkRight(z); 
        }
        return ind;
    }
 
    int InsertNode(Query *q) {
        Node *new_node;
        int new_ind = node.Insert(Node(GetCreateNodeKey(q), 0, 0))+1;
        if (int ind = head) {
            for (;;) {
                Node *n = &node[ind-1];
                if      (n->MoreThan(q->key, ind, &q->z)) { if (!n->left)  { n->left  = new_ind; break; } ind = n->left;  }
                else if (n->LessThan(q->key, ind, &q->z)) { if (!n->right) { n->right = new_ind; break; } ind = n->right; }
                else { node.Erase(new_ind-1); return ResolveInsertCollision(ind, q); }
            }
            (new_node = &node[new_ind-1])->parent = ind;
        } else new_node = &node[(head = new_ind)-1];
        new_node->val = val.Insert(*q->val);
        ComputeStateFromChildrenOnPath(q);
        InsertBalance(new_ind);
        count++;
        return new_ind;
    }
    void InsertBalance(int ind) {
        int p_ind, gp_ind, u_ind;
        Node *n = &node[ind-1], *p, *gp, *u;
        if (!(p_ind = n->parent)) { n->color = Black; return; }
        if ((p = &node[p_ind-1])->color == Black || !(gp_ind = p->parent)) return;
        gp = &node[gp_ind-1];
        if ((u_ind = GetSibling(gp, p_ind)) && (u = &node[u_ind-1])->color == Red) {
            p->color = u->color = Black;
            gp->color = Red;
            return InsertBalance(gp_ind);
        }
        do {
            if      (ind == p->right && p_ind == gp->left)  { RotateLeft (p_ind); ind = node[ind-1].left; }
            else if (ind == p->left  && p_ind == gp->right) { RotateRight(p_ind); ind = node[ind-1].right; }
            else break;
            p  = &node[(p_ind  = GetParent(ind))-1];
            gp = &node[(gp_ind = p->parent)     -1];
        } while (0);
        p->color = Black;
        gp->color = Red;
        if (ind == p->left && p_ind == gp->left) RotateRight(gp_ind);
        else                                     RotateLeft (gp_ind);
    }

    bool EraseNode(Query *q) {
        Node *n;
        int ind = head;
        while (ind) {
            n = &node[ind-1];
            if      (n->MoreThan(q->key, ind, &q->z)) ind = n->left;
            else if (n->LessThan(q->key, ind, &q->z)) ind = n->right;
            else break;
        }
        if (!ind) return false;
        int orig_ind = ind, max_path = 0;
        val.Erase(n->val);
        if (n->left && n->right) {
            n->SwapKV(&node[(ind = GetMaxNode((max_path = n->left)))-1]);
            n = &node[ind-1];
        }
        bool balance = n->color == Black;
        int child = X_or_Y(n->left, n->right), parent = n->parent;
        ReplaceNode(ind, child);
        node.Erase(ind-1);
        if (child == head && head) node[head-1].color = Black;
        if (max_path) ComputeStateFromChildrenOnMaxPath(max_path);
        if (max_path) ComputeStateFromChildren(&node[orig_ind-1]);
        ComputeStateFromChildrenOnPath(q);
        if (balance) EraseBalance(child, parent);
        count--;
        return true;
    }
    void EraseBalance(int ind, int p_ind) {
        if (!p_ind) return;
        Node *p = &node[p_ind-1];
        int s_ind = GetSibling(p, ind);
        if (!s_ind) return;
        Node *s = &node[s_ind-1];
        if (s->color == Red) {
            p->color = Red;
            s->color = Black;
            if (ind == p->left) RotateLeft (p_ind);
            else                RotateRight(p_ind);
            s = &node[(s_ind = GetSibling(p, ind))-1];
        }
        if (s->color == Black) {
            bool slr = GetColor(s->left)  == Red, slb = !slr, lc = ind == p->left;
            bool srr = GetColor(s->right) == Red, srb = !srr, rc = ind == p->right;
            if (p->color == Black && slb && srb) { s->color = Red; return EraseBalance(p_ind, GetParent(p_ind)); }
            if (p->color == Red   && slb && srb) { s->color = Red; p->color = Black; return; }
            if      (lc && slr && srb) { s->color = Red; SetColor(s->left , Black); RotateRight(s_ind); }
            else if (rc && slb && srr) { s->color = Red; SetColor(s->right, Black); RotateLeft (s_ind); }
            s = &node[(s_ind = GetSibling(p, ind))-1];
        }
        s->color = p->color;
        p->color = Black;
        if (ind == p->left) { SetColor(s->right, Black); RotateLeft (p_ind); }
        else                { SetColor(s->left,  Black); RotateRight(p_ind); }
    }

    void ReplaceNode(int ind, int new_ind) {
        Node *n = &node[ind-1];
        if (!n->parent) head = new_ind;
        else if (Node *parent = &node[n->parent-1]) {
            if (ind == parent->left) parent->left  = new_ind;
            else                     parent->right = new_ind;
        }
        if (new_ind) node[new_ind-1].parent = n->parent;
    }
    void RotateLeft(int ind) {
        int right_ind;
        Node *n = &node[ind-1], *o = &node[(right_ind = n->right)-1];
        ReplaceNode(ind, right_ind);
        n->right = o->left;
        if (o->left) node[o->left-1].parent = ind;
        o->left = ind;
        n->parent = right_ind;
        ComputeStateFromChildren(n);
        ComputeStateFromChildren(o);
    }
    void RotateRight(int ind) {
        int left_ind;
        Node *n = &node[ind-1], *o = &node[(left_ind = n->left)-1];
        ReplaceNode(ind, left_ind);
        n->left = o->right;
        if (o->right) node[o->right-1].parent = ind;
        o->right = ind;
        n->parent = left_ind;
        ComputeStateFromChildren(n);
        ComputeStateFromChildren(o);
    }
    void ComputeStateFromChildren(Node *n) {
        n->ComputeStateFromChildren(n->left ? &node[n->left -1] : 0, n->right ? &node[n->right-1] : 0);
    }
    void ComputeStateFromChildrenOnMaxPath(int ind) {
        Node *n = &node[ind-1];
        if (n->right) ComputeStateFromChildrenOnMaxPath(n->right);
        ComputeStateFromChildren(n);
    }
    void SetColor(int ind, int color) { node[ind-1].color = color; }
    int GetColor  (int ind) const { return ind ? node[ind-1].color : Black; }
    int GetParent (int ind) const { return node[ind-1].parent; }
    int GetSibling(int ind) const { return GetSibling(&node[GetParent(ind)-1], ind); }
    int GetMinNode(int ind) const {
        const Node *n = &node[ind-1];
        return n->left ? GetMinNode(n->left) : ind;
    }
    int GetMaxNode(int ind, Zipper *z=0) const {
        const Node *n = &node[ind-1];
        return n->right ? GetMaxNode(n->WalkRight(z), z) : ind;
    }

    struct LoadFromSortedArraysQuery { const K *k; const V *v; int max_height; };
    virtual void LoadFromSortedArrays(const K *k, const V *v, int n) {
        CHECK_EQ(0, node.size());
        LoadFromSortedArraysQuery q = { k, v, WhichLog2(NextPowerOfTwo(n, true)) };
        count = n;
        head = BuildTreeFromSortedArrays(&q, 0, n-1, 1);
    }
    virtual int BuildTreeFromSortedArrays(LoadFromSortedArraysQuery *q, int beg_ind, int end_ind, int h) {
        if (end_ind < beg_ind) return 0;
        CHECK_LE(h, q->max_height);
        int mid_ind = (beg_ind + end_ind) / 2, color = ((h>1 && h == q->max_height) ? Red : Black);
        int node_ind = node.Insert(Node(q->k[mid_ind], val.Insert(q->v[mid_ind]), 0, color))+1;
        int left_ind  = node[node_ind-1].left  = BuildTreeFromSortedArrays(q, beg_ind,   mid_ind-1, h+1);
        int right_ind = node[node_ind-1].right = BuildTreeFromSortedArrays(q, mid_ind+1, end_ind,   h+1);
        if (left_ind)  node[ left_ind-1].parent = node_ind;
        if (right_ind) node[right_ind-1].parent = node_ind;
        ComputeStateFromChildren(&node[node_ind-1]);
        return node_ind;
    }

    void CheckProperties() const {
        CHECK_EQ(Black, GetColor(head));
        CheckNoAdjacentRedNodes(head);
        int same_black_length = -1;
        CheckEveryPathToLeafHasSameBlackLength(head, 0, &same_black_length);
    }
    void CheckNoAdjacentRedNodes(int ind) const {
        if (!ind) return;
        const Node *n = &node[ind-1];
        CheckNoAdjacentRedNodes(n->left);
        CheckNoAdjacentRedNodes(n->right);
        if (n->color == Black) return;
        CHECK_EQ(Black, GetColor(n->parent));
        CHECK_EQ(Black, GetColor(n->parent));
        CHECK_EQ(Black, GetColor(n->parent));
    }
    void CheckEveryPathToLeafHasSameBlackLength(int ind, int black_length, int *same_black_length) const {
        if (!ind) {
            if (*same_black_length < 0) *same_black_length = black_length;
            CHECK_EQ(*same_black_length, black_length);
            return;
        }
        const Node *n = &node[ind-1];
        if (n->color == Black) black_length++;
        CheckEveryPathToLeafHasSameBlackLength(n->left,  black_length, same_black_length);
        CheckEveryPathToLeafHasSameBlackLength(n->right, black_length, same_black_length);
    }

    virtual string DebugString(const string &name=string()) const {
        string ret = GraphViz::DigraphHeader(StrCat("RedBlackTree", name.size()?"_":"", name));
        ret += GraphViz::NodeColor("black"); PrintNodes(head, Black, &ret);
        ret += GraphViz::NodeColor("red");   PrintNodes(head, Red,   &ret);
        PrintEdges(head, &ret);
        return ret + GraphViz::Footer();
    }
    virtual void PrintNodes(int ind, int color, string *out) const {
        if (!ind) return;
        const Node *n = &node[ind-1];
        PrintNodes(n->left, color, out);
        if (n->color == color) GraphViz::AppendNode(out, StrCat(n->key));
        PrintNodes(n->right, color, out);
    }
    virtual void PrintEdges(int ind, string *out) const {
        if (!ind) return;
        const Node *n = &node[ind-1], *l=n->left?&node[n->left-1]:0, *r=n->right?&node[n->right-1]:0;
        if (l) { PrintEdges(n->left, out);  GraphViz::AppendEdge(out, StrCat(n->key), StrCat(l->key), "left" ); }
        if (r) { PrintEdges(n->right, out); GraphViz::AppendEdge(out, StrCat(n->key), StrCat(r->key), "right"); }
    }

    static int GetSibling(const Node *parent, int ind) { return ind == parent->left ? parent->right : parent->left; }
};

struct PrefixSumZipper {
    int sum=0;
    bool update;
    vector<pair<int, bool> > path;
    PrefixSumZipper(bool U=0) : update(U) { if (update) path.reserve(64); }
};

template <class K, class V, class Zipper = PrefixSumZipper>
struct PrefixSumKeyedRedBlackTreeNode : public RedBlackTreeNode<K,V,Zipper> {
    typedef RedBlackTreeNode<K,V,Zipper> Parent;
    typedef PrefixSumKeyedRedBlackTreeNode<K,V,Zipper> Self;
    int left_sum=0, right_sum=0;
    PrefixSumKeyedRedBlackTreeNode(K k=K(), unsigned v=0, unsigned p=0, bool c=0) : RedBlackTreeNode<K,V,Zipper>(k,v,p,c) {}
    virtual K GetKey(PrefixSumZipper *z) const { return z->sum + left_sum + Parent::key; }
    virtual int WalkRight  (Zipper *z) const { if (z) z->sum += (left_sum + Parent::key); return Parent::right; }
    virtual int UnwalkRight(Zipper *z) const { if (z) z->sum -= (left_sum + Parent::key); return Parent::parent; }
    virtual bool LessThan(const K &k, int ind, PrefixSumZipper *z) const {
        if (!(GetKey(z) < k)) return 0;
        if (z->update) z->path.emplace_back(ind, Parent::Left);
        WalkRight(z);
        return 1;
    }
    virtual bool MoreThan(const K &k, int ind, PrefixSumZipper *z) const {
        if (!(k < GetKey(z))) return 0;
        if (z->update) z->path.emplace_back(ind, Parent::Right);
        return 1;
    }
    virtual void ComputeStateFromChildren(const Self *lc, const Self *rc) {
        Parent::ComputeStateFromChildren(lc, rc);
        left_sum  = lc ? (lc->left_sum + lc->right_sum + lc->key) : 0;
        right_sum = rc ? (rc->left_sum + rc->right_sum + rc->key) : 0;
    }
};

template <class K, class V, class Zipper = PrefixSumZipper, class Node = PrefixSumKeyedRedBlackTreeNode<K,V,Zipper> >
struct PrefixSumKeyedRedBlackTree : public RedBlackTree<K,V,PrefixSumZipper,Node> {
    typedef RedBlackTree<K,V,PrefixSumZipper,Node> Parent;
    function<K(const V*)> node_value_cb = bind([&](){ return 1; });

    virtual K GetCreateNodeKey(const typename Parent::Query *q) const { return node_value_cb(q->val); }
    virtual typename Parent::Query GetQuery(const K &k, const V *v=0, bool update=0) const {
        return typename Parent::Query(k + (v ? 0 : 1), v, update);
    }
    virtual int ResolveInsertCollision(int ind, typename Parent::Query *q) { 
        int val_ind = Parent::val.Insert(*q->val), p_ind = ind;
        int new_ind = Parent::node.Insert(Node(GetCreateNodeKey(q), val_ind))+1;
        Node *n = &Parent::node[ind-1], *nn;
        if (!n->left) n->left = new_ind;
        else Parent::node[(p_ind = Parent::GetMaxNode(n->left))-1].right = new_ind;
        n->SwapKV((nn = &Parent::node[new_ind-1]));
        nn->parent = p_ind;
        Parent::ComputeStateFromChildrenOnMaxPath(n->left);
        Parent::ComputeStateFromChildren(n);
        ComputeStateFromChildrenOnPath(q);
        Parent::InsertBalance(new_ind);
        Parent::count++;
        return new_ind;
    }
    virtual void ComputeStateFromChildrenOnPath(typename Parent::Query *q) {
        for (auto i = q->z.path.rbegin(), e = q->z.path.rend(); i != e; ++i) 
            Parent::ComputeStateFromChildren(&Parent::node[i->first-1]);
    }

    virtual void PrintNodes(int ind, int color, string *out) const {
        if (!ind) return;
        const Node *n = &Parent::node[ind-1];
        const V    *v = &Parent::val[n->val];
        PrintNodes(n->left, color, out);
        if (n->color == color) StrAppend(out, "node [label = \"", *v, " v:", n->key, "\nlsum:", n->left_sum,
                                         " rsum:", n->right_sum, "\"];\r\n\"", *v, "\";\r\n");
        PrintNodes(n->right, color, out);
    }
    virtual void PrintEdges(int ind, string *out) const {
        if (!ind) return;
        const Node *n = &Parent::node[ind-1], *l=n->left?&Parent::node[n->left-1]:0, *r=n->right?&Parent::node[n->right-1]:0;
        const V    *v = &Parent::val[n->val], *lv = l?&Parent::val[l->val]:0, *rv = r?&Parent::val[r->val]:0;
        if (l) { PrintEdges(n->left,  out); StrAppend(out, "\"", *v, "\" -> \"", *lv, "\" [ label = \"left\"  ];\r\n"); }
        if (r) { PrintEdges(n->right, out); StrAppend(out, "\"", *v, "\" -> \"", *rv, "\" [ label = \"right\" ];\r\n"); }
   }

    void LoadFromSortedVal() {
        int n = Parent::val.size();
        CHECK_EQ(0, Parent::node.size());
        Parent::count = Parent::val.size();
        Parent::head = BuildTreeFromSortedVal(0, n-1, 1, WhichLog2(NextPowerOfTwo(n, true)));
    }
    int BuildTreeFromSortedVal(int beg_val_ind, int end_val_ind, int h, int max_h) {
        if (end_val_ind < beg_val_ind) return 0;
        CHECK_LE(h, max_h);
        int mid_val_ind = (beg_val_ind + end_val_ind) / 2, color = ((h>1 && h == max_h) ? Parent::Red : Parent::Black);
        int ind = Parent::node.Insert(Node(node_value_cb(&Parent::val[mid_val_ind]), mid_val_ind, 0, color))+1;
        int left_ind  = Parent::node[ind-1].left  = BuildTreeFromSortedVal(beg_val_ind,   mid_val_ind-1, h+1, max_h);
        int right_ind = Parent::node[ind-1].right = BuildTreeFromSortedVal(mid_val_ind+1, end_val_ind,   h+1, max_h);
        if (left_ind)  Parent::node[ left_ind-1].parent = ind;
        if (right_ind) Parent::node[right_ind-1].parent = ind;
        Parent::ComputeStateFromChildren(&Parent::node[ind-1]);
        return ind;
    }

    V *Update(const K &k, const V &v) {
        typename Parent::Query q = GetQuery(k, 0, 1); 
        Node *n;
        for (int ind = Parent::head; ind; ) {
            n = &Parent::node[ind-1];
            if      (n->MoreThan(q.key, ind, &q.z)) ind = n->left;
            else if (n->LessThan(q.key, ind, &q.z)) ind = n->right;
            else {
                n->key = node_value_cb(&v);
                ComputeStateFromChildrenOnPath(&q);
            }
        } return 0;
    }
};

}; // namespace LFL
#endif // __LFL_LFAPP_TREE_H__