/*
 * $Id: lfapp.cpp 1309 2014-10-10 19:20:55Z justin $
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

#include "gtest/gtest.h"
#include "lfapp/lfapp.h"

GTEST_API_ int main(int argc, const char **argv) {
    testing::InitGoogleTest(&argc, (char**)argv);
    LFL::FLAGS_default_font = LFL::FakeFont::Filename();
    CHECK_EQ(LFL::app->Create(argc, argv, __FILE__), 0);
    return RUN_ALL_TESTS();
}

namespace LFL {
DEFINE_int(size,         1024*1024, "Test size"); 
DEFINE_int(rand_key_min, 0,         "Min key");
DEFINE_int(rand_key_max, 10000000,  "Max key");

void PrintGraphVizFooter() { printf("}\r\n"); }
void PrintGraphVizHeader(const char *name) {
    printf("digraph %s {\r\n"
           "rankdir=LR;\r\n"
           "size=\"8,5\"\r\n"
           "node [style = solid];\r\n"
           "node [shape = circle];\r\n", name);
}

struct MyEnvironment : public ::testing::Environment {
    vector<int> rand_db, incr_db, decr_db, sorted_rand_db, sorted_incr_db, sorted_decr_db;
    vector<Triple<string, vector<int>*, vector<int>*> > db;

    virtual ~MyEnvironment() {}
    virtual void TearDown() { INFO(Singleton<PerformanceTimers>::Get()->DebugString()); }
    virtual void SetUp() { 
        unordered_set<int> rand_unique;
        for (int i=0; i<FLAGS_size; i++) {
            int ri = Rand(FLAGS_rand_key_min, FLAGS_rand_key_max);
            if (!rand_unique.insert(ri).second) { i--; continue; }
            rand_db.push_back(ri);
        }
        for (int i=0; i<FLAGS_size; i++) incr_db.push_back(i);
        for (int i=0; i<FLAGS_size; i++) decr_db.push_back(FLAGS_size-i);

        sorted_rand_db = rand_db; sort(sorted_rand_db.begin(), sorted_rand_db.end());
        sorted_incr_db = incr_db; sort(sorted_incr_db.begin(), sorted_incr_db.end());
        sorted_decr_db = decr_db; sort(sorted_decr_db.begin(), sorted_decr_db.end());

        db.emplace_back("rand", &rand_db, &sorted_rand_db);
        db.emplace_back("incr", &incr_db, &sorted_incr_db);
        db.emplace_back("decr", &decr_db, &sorted_decr_db);
    }
};

MyEnvironment* const my_env = (MyEnvironment*)::testing::AddGlobalTestEnvironment(new MyEnvironment);

template <class K, class V> struct SkipList {
    struct Node {
        K key; int val_ind, next_ind, leveldown_ind;
        Node(K k=K(), int VI=-1, int NI=-1, int LDI=-1) : key(k), val_ind(VI), next_ind(NI), leveldown_ind(LDI) {}
    };
    FreeListVector<V> val;
    FreeListVector<Node> node;
    int count=0, head_ind=-1;

    void Insert(const K &k, const V &v) {
        vector<pair<int, int> > zipper;
        zipper.reserve(64);
        int ind = FindNode(k, &zipper), next_ind;
        if (ind < 0 || (next_ind = node[ind].next_ind) < 0) {}
        else if (const Node *n = &node[next_ind]) if (k == n->key) { val[n->val_ind] = v; return; }
        int val_ind = val.Insert(v), level = RandomLevel();
        for (int i=0, zl=zipper.size(), last_ind=-1; i<level; i++) {
            int zi = i < zl ? zl-i-1 : -1;
            last_ind = node.Insert(Node(k, val_ind, zi < 0 ? -1 : zipper[zi].second, last_ind));
            if (zi >= 0) node[zipper[zi].first].next_ind = last_ind;
            else head_ind = node.Insert(Node(-1, -1, last_ind, head_ind));
        }
        count++;
    }
    bool Erase(const K &k) {
        vector<pair<int, int> > zipper;
        zipper.reserve(64);
        int ind = FindNode(k, &zipper), next_ind, val_ind, erase_ind;
        if (ind < 0 || (next_ind = node[ind].next_ind) < 0) return false;
        const Node *n = &node[next_ind];
        if (k != n->key) return false;
        val.Erase((val_ind = n->val_ind));
        for (int i=zipper.size()-1; i>=0; i--) {
            Node *p = &node[zipper[i].first];
            if ((erase_ind = p->next_ind) < 0 || (n = &node[erase_ind])->val_ind != val_ind) break;
            p->next_ind = n->next_ind;
            node.Erase(erase_ind);
        }
        CHECK(count);
        return count--;
    }
    V *Find(const K &k) {
        int ind = FindNode(k), next_ind;
        if (ind < 0 || (next_ind = node[ind].next_ind) < 0) return 0;
        const Node *n = &node[next_ind];
        return k == n->key ? &val[n->val_ind] : 0;
    }
    int FindNode(const K &k, vector<pair<int, int> > *zipper=0) const {
        int ind = head_ind, next_ind;
        for (int leveldown_ind; ind >= 0; ind = leveldown_ind) {
            const Node *n = &node[ind], *nn;
            while (n->next_ind >= 0 && (nn = &node[n->next_ind])->key < k) { ind = n->next_ind; n = nn; }
            if (zipper) zipper->emplace_back(ind, n->next_ind);
            if ((leveldown_ind = n->leveldown_ind) < 0) break;
        }
        return ind;
    }
    string DebugString() const {
        string v;
        for (int i=0, ind=head_ind; ind >= 0; ind = node[ind].leveldown_ind, i--) {
            StrAppend(&v, "\n", i, ": H");
            for (int pi = node[ind].next_ind; pi >= 0; pi = node[pi].next_ind)
                StrAppend(&v, " ", node[pi].val_ind < 0 ? "-1" : StrCat(val[node[pi].val_ind]));
        } return v;
    }
    static int RandomLevel() { static float logP = log(.5); return 1 + (int)(log(Rand(0,1)) / logP); }
};

template <class K, class V> struct AVLTreeNode {
    K key; unsigned val:30, left:30, right:30, height:6;
    AVLTreeNode(K k, unsigned v) : key(k), val(v), left(0), right(0), height(1) {}
    void SetChildren(unsigned l, unsigned r) { left=l; right=r; }
};

template <class K, class V, class Node = AVLTreeNode<K,V> > struct AVLTree {
    struct Query {
        const K &key; const V *val; V *ret;
        Query(const K &k, const V *v=0) : key(k), val(v), ret(0) {}
    };

    FreeListVector<Node> node;
    FreeListVector<V> val;
    int head=0;

    const V* Find  (const K &k) const       { Query q(k); int vind = Find  (head, &q); return vind ? &val[vind-1] : 0; }
    V*       Find  (const K &k)             { Query q(k); int vind = Find  (head, &q); return vind ? &val[vind-1] : 0; }
    V*       Insert(const K &k, const V &v) { Query q(k, &v); head = Insert(head, &q); return q.ret; }
    bool     Erase (const K &k)             { Query q(k);     head = Erase (head, &q); return q.ret; }
    void     Print() const { PrintGraphVizHeader("AVLTree"); Print(head); PrintGraphVizFooter(); }

    int Find(int ind, Query *q) const {
        if (!ind) return 0;
        const Node *n = &node[ind-1];
        if      (q->key < n->key) return Find(n->left,  q);
        else if (n->key < q->key) return Find(n->right, q);
        else return n->val+1;
    }
    int Insert(int ind, Query *q) {
        if (!ind) return Create(q);
        Node *n = &node[ind-1];
        if      (q->key < n->key) node[ind-1].left  = Insert(n->left,  q);
        else if (n->key < q->key) node[ind-1].right = Insert(n->right, q);
        else { q->ret = &(val[n->val] = *q->val); return ind; }
        return Balance(ind);
    }
    int Create(Query *q) {
        int val_ind = val.Insert(*q->val);
        q->ret = &val[val_ind];
        return node.Insert(Node(q->key, val_ind))+1;
    }
    int Erase(int ind, Query *q) {
        if (!ind) return 0;
        Node *n = &node[ind-1];
        if      (q->key < n->key) n->left  = Erase(n->left,  q);
        else if (n->key < q->key) n->right = Erase(n->right, q);
        else {
            q->ret = &val[n->val];
            int left = n->left, right = n->right;
            val.Erase(n->val);
            node.Erase(ind-1);
            if (!right) return left;
            ind = GetMinNode(right);
            right = EraseMinNode(right);
            node[ind-1].SetChildren(left, right);
        }
        return Balance(ind);
    }
    int EraseMinNode(int ind) {
        Node *n = &node[ind-1];
        if (!n->left) return n->right;
        n->left = EraseMinNode(n->left);
        return Balance(ind);
    }
    int Balance(int ind) {
        Node *n = &node[ind-1];
        n->height = GetHeight(n);
        int nbal = GetBalance(ind);
        if (GetBalance(ind) > 1) {
            if (GetBalance(n->right) < 0) n->right = RotateRight(n->right);
            return RotateLeft(ind);
        } else if (nbal < -1) {
            if (GetBalance(n->left) > 0) n->left = RotateLeft(n->left);
            return RotateRight(ind);
        } else return ind;
    }
    int RotateLeft(int ind) {
        int right_ind;
        Node *n = &node[ind-1], *o = &node[(right_ind = n->right)-1];
        n->right = o->left;
        o->left = ind;
        n->height = GetHeight(n);
        o->height = GetHeight(o);
        return right_ind;
    }
    int RotateRight(int ind) {
        int left_ind;
        Node *n = &node[ind-1], *o = &node[(left_ind = n->left)-1];
        n->left = o->right;
        o->right = ind;
        n->height = GetHeight(n);
        o->height = GetHeight(o);
        return left_ind;
    }
    int GetMinNode(int ind) const {
        const Node *n = &node[ind-1];
        return n->left ? GetMinNode(n->left) : ind;
    }
    int GetHeight(Node *n) const {
        return max(n->left  ? node[n->left -1].height : 0,
                   n->right ? node[n->right-1].height : 0) + 1;
    }
    int GetBalance(int ind) const {
        const Node *n = &node[ind-1];
        return (n->right ? node[n->right-1].height : 0) -
               (n->left  ? node[n->left -1].height : 0);
    }
    void Print(int ind) const {
        if (!ind) return;
        const Node *n = &node[ind-1], *l=n->left?&node[n->left-1]:0, *r=n->right?&node[n->right-1]:0;
        if (l) { Print(n->left);  printf("\"%d\" -> \"%d\" [ label = \"left\"  ];\r\n", n->key, l->key); }
        if (r) { Print(n->right); printf("\"%d\" -> \"%d\" [ label = \"right\" ];\r\n", n->key, r->key); }
    }
};

template <class K, class V> struct RedBlackTreeNode {
    K key; unsigned val:31, left, right, parent, color:1;
    RedBlackTreeNode(K k, unsigned v) : key(k), val(v), left(0), right(0), parent(0), color(0) {}
    void Assign(K k, unsigned v) { key=k; val=v; }
};

template <class K, class V, class Node = RedBlackTreeNode<K,V> > struct RedBlackTree {
    enum Color { Red, Black };

    FreeListVector<Node> node;
    FreeListVector<V> val;
    int head=0;

    const V* Find (const K &k) const { const Node *n = FindNode(k); return n ? &val[n->val] : 0; }
    /**/  V* Find (const K &k)       { const Node *n = FindNode(k); return n ? &val[n->val] : 0; }
    const Node *FindNode(const K &k) const {
        for (int ind = head; ind;) {
            const Node *n = &node[ind-1];
            if      (k < n->key) ind = n->left;
            else if (n->key < k) ind = n->right;
            else return n;
        } return 0;
    }

    V* Insert(const K &k, const V &v) {
        Node *new_node;
        int new_ind = node.Insert(Node(k, 0))+1, val_ind;
        if (int ind = head) {
            for (;;) {
                Node *n = &node[ind-1];
                if      (k < n->key) { if (!n->left)  { n->left  = new_ind; break; } ind = n->left;  }
                else if (n->key < k) { if (!n->right) { n->right = new_ind; break; } ind = n->right; }
                else                 { node.Erase(new_ind-1); return &(val[n->val] = v); }
            }
            (new_node = &node[new_ind-1])->parent = ind;
        } else new_node = &node[(head = new_ind)-1];
        new_node->val = val_ind = val.Insert(v);
        InsertCase1(new_ind);
        return &val[val_ind];
    }
    void InsertCase1(int ind) {
        Node *n = &node[ind-1];
        if (n->parent) InsertCase2(ind);
        else n->color = Color::Black;
    }
    void InsertCase2(int ind) { if (GetColor(node[ind-1].parent) == Color::Red) InsertCase3(ind); }
    void InsertCase3(int ind) {
        int uncle_ind, parent_ind, grandparent_ind;
        if (GetColor((uncle_ind = GetUncle(ind))) == Color::Black) return InsertCase4(ind);
        node[(parent_ind = GetParent(ind))-1].color = node[uncle_ind-1].color = Color::Black;
        node[(grandparent_ind = GetParent(parent_ind))-1].color = Color::Red;
        InsertCase1(grandparent_ind);
    }
    void InsertCase4(int ind) {
        int p_ind = GetParent(ind);
        Node *p = &node[p_ind-1];
        if      (ind == p->right && p_ind == node[p->parent-1].left)  { RotateLeft (p_ind); ind = node[ind-1].left; }
        else if (ind == p->left  && p_ind == node[p->parent-1].right) { RotateRight(p_ind); ind = node[ind-1].right; }
        InsertCase5(ind);
    }
    void InsertCase5(int ind) {
        int p_ind = GetParent(ind), gp_ind;
        Node *p = &node[p_ind-1], *gp = &node[(gp_ind = p->parent)-1];
        p->color = Color::Black;
        gp->color = Color::Red;
        if (ind == p->left && p_ind == gp->left) RotateRight(gp_ind);
        else                                     RotateLeft (gp_ind);
    }

    bool Erase(const K &k) {
        Node *n = (Node*)FindNode(k);
        if (!n) return false;
        val.Erase(n->val);
        int ind = n - &node.data[0] + 1, m_ind;
        if (n->left && n->right) {
            Node *m = &node[(m_ind = GetMaxNode(n->left))-1];
            n->Assign(m->key, m->val);
            n = &node[(ind = m_ind)-1];
        }
        int child = X_or_Y(n->right, n->left);
        if (n->color == Color::Black) {
            n->color = GetColor(child);
            EraseCase1(ind);
        }
        ReplaceNode(ind, child);
        node.Erase(ind-1);
        return true;
    }
    void EraseCase1(int ind) {
        Node *n = &node[ind-1];
        if (n->parent) EraseCase2(ind);
    }
    void EraseCase2(int ind) {
        Node *p, *s;
        int p_ind, s_ind;
        if ((s_ind = GetSibling(ind)) && (s = &node[s_ind-1])->color == Color::Red) {
            p = &node[(p_ind = GetParent(ind))-1];
            p->color = Color::Red;
            s->color = Color::Black;
            if (ind == p->left) RotateLeft (p_ind);
            else                RotateRight(p_ind);
        }
        EraseCase3(ind);
    }
    void EraseCase3(int ind) {
        int p_ind = GetParent(ind), s_ind = GetSibling(ind);
        Node *s = &node[s_ind-1];
        if (GetColor(p_ind)   == Color::Black && s->color           == Color::Black &&
            GetColor(s->left) == Color::Black && GetColor(s->right) == Color::Black) {
            s->color = Color::Red;
            EraseCase1(p_ind);
        } else EraseCase4(ind);
    }
    void EraseCase4(int ind) {
        int p_ind = GetParent(ind), s_ind = GetSibling(ind);
        Node *p = &node[p_ind-1], *s = &node[s_ind-1];
        if (GetColor(p_ind)   == Color::Red   && s->color           == Color::Black &&
            GetColor(s->left) == Color::Black && GetColor(s->right) == Color::Black) {
            s->color = Color::Red;
            p->color = Color::Black;
        } else EraseCase5(ind);
    } 
    void EraseCase5(int ind) {
        int p_ind = GetParent(ind), s_ind = GetSibling(ind);
        Node *p = &node[p_ind-1], *s = &node[s_ind-1];
        if (s->color == Color::Black) {
            int csl = GetColor(s->left), csr = GetColor(s->right);
            if      (ind == p->left  && csl == Color::Red   && csr == Color::Black) { s->color = Color::Red; node[s->left -1].color = Color::Black; RotateRight(s_ind); }
            else if (ind == p->right && csl == Color::Black && csr == Color::Red  ) { s->color = Color::Red; node[s->right-1].color = Color::Black; RotateLeft (s_ind); }
        }
        EraseCase6(ind);
    }
    void EraseCase6(int ind) {
        int p_ind = GetParent(ind), s_ind = GetSibling(ind);
        Node *n = &node[ind-1], *p = &node[p_ind-1], *s = &node[s_ind-1];
        s->color = p->color;
        p->color = Color::Black;
        if (ind == p->left) { node[s->right-1].color = Color::Black; RotateLeft (p_ind); }
        else                { node[s->left -1].color = Color::Black; RotateRight(p_ind); }
    }

    void RotateLeft (int ind) {
        int right_ind;
        Node *n = &node[ind-1], *o = &node[(right_ind = n->right)-1];
        ReplaceNode(ind, right_ind);
        n->right = o->left;
        if (o->left) node[o->left-1].parent = ind;
        o->left = ind;
        n->parent = right_ind;
    }
    void RotateRight(int ind) {
        int left_ind;
        Node *n = &node[ind-1], *o = &node[(left_ind = n->left)-1];
        ReplaceNode(ind, left_ind);
        n->left = o->right;
        if (o->right) node[o->right-1].parent = ind;
        o->right = ind;
        n->parent = left_ind;
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

    int GetColor  (int ind) const { return ind ? node[ind-1].color : Color::Black; }
    int GetParent (int ind) const { return node[ind-1].parent; }
    int GetUncle  (int ind) const { return GetSibling(GetParent(ind)); }
    int GetSibling(int ind) const {
        const Node *parent = &node[GetParent(ind)-1];
        return ind == parent->left ? parent->right : parent->left;
    }
    int GetMaxNode(int ind) const {
        const Node *n = &node[ind-1];
        return n->right ? GetMaxNode(n->right) : ind;
    }

    void Print() const { 
        PrintGraphVizHeader("RedBlackTree"); 
        printf("node [color = black];\r\n"); PrintNodes(head, Color::Black);
        printf("node [color = red];\r\n");   PrintNodes(head, Color::Red);
        PrintEdges(head);
        PrintGraphVizFooter();
    }
    void PrintNodes(int ind, int color) const {
        if (!ind) return;
        const Node *n = &node[ind-1];
        PrintColor(n->left, color);
        if (n->color == color) printf("\"%d\";\r\n", n->key);
        PrintColor(n->right, color);
    }
    void PrintEdges(int ind) const {
        if (!ind) return;
        const Node *n = &node[ind-1], *l=n->left?&node[n->left-1]:0, *r=n->right?&node[n->right-1]:0;
        if (l) { PrintEdges(n->left);  printf("\"%d\" -> \"%d\" [ label = \"left\"  ];\r\n", n->key, l->key); }
        if (r) { PrintEdges(n->right); printf("\"%d\" -> \"%d\" [ label = \"right\" ];\r\n", n->key, r->key); }
    }
};

template <class K, class V> struct PrefixSumRedBlackTreeNode : public RedBlackTreeNode<K,V> {
    V sum=V();
    PrefixSumRedBlackTreeNode(K k=K(), unsigned v=0) : RedBlackTreeNode<K,V>(k,v) {}
};

template <class K, class V, class Node = PrefixSumRedBlackTreeNode<K,V> >
struct PrefixSumRedBlackTree : public RedBlackTree<K,V,Node> {
};

TEST(DatastructureTest, StdMap) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second, &sorted_db = *i.third;
        map<int, int> m;
        map<int, int>::const_iterator mi;
        int ctid = timers->Create(StrCat("std::map  ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("std::map  ", i.first, " query1"));
        int itid = timers->Create(StrCat("std::map  ", i.first, " iterO1")), iind=0;
        int dtid = timers->Create(StrCat("std::map  ", i.first, " del   "));
        int rtid = timers->Create(StrCat("std::map  ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("std::map  ", i.first, " ins2  "));

        timers->AccumulateTo(ctid); for (auto i : db) m[i] = i;
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE(m.end(), (mi=m.find(i))); if (mi!=m.end()) EXPECT_EQ(i, mi->second); }
        timers->AccumulateTo(itid); for (auto i : m)  { EXPECT_EQ(sorted_db[iind], i.first); EXPECT_EQ(sorted_db[iind], i.second); iind++; }
        timers->AccumulateTo(dtid); for (int i=0, l=db.size()/2; i<l; i++) EXPECT_EQ(1, m.erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(m.end(), m.find(db[i]));
            else { EXPECT_NE(m.end(), (mi=m.find(db[i]))); if (mi!=m.end()) EXPECT_EQ(db[i], mi->second); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) m[db[i]] = db[i];
        timers->AccumulateTo(0);
    }
}

TEST(DatastructureTest, StdUnorderedMap) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second, &sorted_db = *i.third;
        unordered_map<int, int> m;
        unordered_map<int, int>::const_iterator mi;
        int ctid = timers->Create(StrCat("std::umap ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("std::umap ", i.first, " query1"));
        int itid = timers->Create(StrCat("std::umap ", i.first, " iterU1")), iind=0;
        int dtid = timers->Create(StrCat("std::umap ", i.first, " del   "));
        int rtid = timers->Create(StrCat("std::umap ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("std::umap ", i.first, " ins2  "));

        timers->AccumulateTo(ctid); for (auto i : db) m[i] = i;
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE(m.end(), (mi=m.find(i))); if (mi!=m.end()) EXPECT_EQ(i, mi->second); }
        timers->AccumulateTo(itid); for (auto i : m) {}
        timers->AccumulateTo(dtid); for (int i=0, l=db.size()/2; i<l; i++) EXPECT_EQ(1, m.erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(m.end(), m.find(db[i]));
            else { EXPECT_NE(m.end(), (mi=m.find(db[i]))); if (mi!=m.end()) EXPECT_EQ(db[i], mi->second); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) m[db[i]] = db[i];
        timers->AccumulateTo(0);
    }
}

#ifdef LFL_JUDY
TEST(DatastructureTest, JudyArray) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second, &sorted_db = *i.third;
        judymap<int, int> m;
        judymap<int, int>::const_iterator mi;
        int ctid = timers->Create(StrCat("JudyArray ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("JudyArray ", i.first, " query1"));
        int itid = timers->Create(StrCat("JudyArray ", i.first, " iterO1")), iind=0;
        int dtid = timers->Create(StrCat("JudyArray ", i.first, " del   "));
        int rtid = timers->Create(StrCat("JudyArray ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("JudyArray ", i.first, " ins2  "));

        timers->AccumulateTo(ctid); for (auto i : db) m[i] = i;
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE(m.end(), (mi=m.find(i))); if (mi!=m.end()) EXPECT_EQ(i, (int)(long)(*mi).second); }
        timers->AccumulateTo(itid); for (auto i : m)  { EXPECT_EQ(sorted_db[iind], i.first); EXPECT_EQ(sorted_db[iind], i.second); iind++; }
        timers->AccumulateTo(dtid); for (int i=0, l=db.size()/2; i<l; i++) EXPECT_EQ(1, m.erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(m.end(), m.find(db[i]));
            else { EXPECT_NE(m.end(), (mi=m.find(db[i]))); if (mi!=m.end()) EXPECT_EQ(db[i], (*mi).second); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) m[db[i]] = db[i];
        timers->AccumulateTo(0);
    }
}
#endif

TEST(DatastructureTest, SkipList) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second;
        SkipList<int, int> t;
        int ctid = timers->Create(StrCat("SkipList  ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("SkipList  ", i.first, " query1"));
        int dtid = timers->Create(StrCat("SkipList  ", i.first, " del   ")); 
        int rtid = timers->Create(StrCat("SkipList  ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("SkipList  ", i.first, " ins2  ")), *v; 

        timers->AccumulateTo(ctid); for (auto i : db) t.Insert(i, i);
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE((int*)0, (v=t.Find(i))); if (v) EXPECT_EQ(i, *v); }
        timers->AccumulateTo(dtid); for (int i=0, hl=db.size()/2; i<hl; i++) EXPECT_EQ(true, t.Erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(0, t.Find(db[i]));
            else { EXPECT_NE((int*)0, (v=t.Find(db[i]))); if (v) EXPECT_EQ(db[i], *v); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) t.Insert(db[i], db[i]);
        timers->AccumulateTo(0);
    }
}

TEST(DatastructureTest, AVLTree) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second;
        AVLTree<int, int> t;
        int ctid = timers->Create(StrCat("AVLTree   ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("AVLTree   ", i.first, " query1"));
        int dtid = timers->Create(StrCat("AVLTree   ", i.first, " del   ")); 
        int rtid = timers->Create(StrCat("AVLTree   ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("AVLTree   ", i.first, " ins2  ")), *v; 
                                                    
        timers->AccumulateTo(ctid); for (auto i : db) t.Insert(i, i);
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE((int*)0, (v=t.Find(i))); if (v) EXPECT_EQ(i, *v); }
        timers->AccumulateTo(dtid); for (int i=0, hl=db.size()/2; i<hl; i++) EXPECT_EQ(true, t.Erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(0, t.Find(db[i]));
            else { EXPECT_NE((int*)0, (v=t.Find(db[i]))); if (v) EXPECT_EQ(db[i], *v); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) t.Insert(db[i], db[i]);
        timers->AccumulateTo(0);
    }
}

TEST(DatastructureTest, RedBlackTree) {
    PerformanceTimers *timers = Singleton<PerformanceTimers>::Get();
    for (auto i : my_env->db) {
        auto &db = *i.second;
        RedBlackTree<int, int> t;
        int ctid = timers->Create(StrCat("RBTree    ", i.first, " ins1  "));
        int qtid = timers->Create(StrCat("RBTree    ", i.first, " query1"));
        int dtid = timers->Create(StrCat("RBTree    ", i.first, " del   ")); 
        int rtid = timers->Create(StrCat("RBTree    ", i.first, " query2"));
        int Ctid = timers->Create(StrCat("RBTree    ", i.first, " ins2  ")), *v; 
                                                    
        timers->AccumulateTo(ctid); for (auto i : db) t.Insert(i, i);
        timers->AccumulateTo(qtid); for (auto i : db) { EXPECT_NE((int*)0, (v=t.Find(i))); if (v) EXPECT_EQ(i, *v); }
        timers->AccumulateTo(dtid); for (int i=0, hl=db.size()/2; i<hl; i++) EXPECT_EQ(true, t.Erase(db[i]));
        timers->AccumulateTo(rtid);
        for (int i=0, l=db.size(), hl=l/2; i<l; i++) {
            if (i < hl) EXPECT_EQ(0, t.Find(db[i]));
            else { EXPECT_NE((int*)0, (v=t.Find(db[i]))); if (v) EXPECT_EQ(db[i], *v); }
        }
        timers->AccumulateTo(Ctid); for (int i=db.size()/2-1; i>=0; i--) t.Insert(db[i], db[i]);
        timers->AccumulateTo(0);
    }
}
}; // namespace LFL