//
// Created by Administrator on 2026/5/13.
//
#pragma once
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>

template <class Key, class Value, int M = 50>
class BPlusTree {
private:
    static const int MIN = (M - 1) / 2;
    static const int MAX_INTERNAL_NODES = 5000;

    struct Leaf {
        int cnt = 0; // 有多少数据
        int next = -1; // 下一个叶节点块号，不是地址
        int prev = -1;
        Key k[M + 1];
        Value v[M + 1];
        void clear() {
            cnt  = 0;
            next = -1;
            prev = -1;
            for (int i = 0; i <= M; i++) {
                k[i] = Key();
                v[i] = Value();
            }
        }
    };

    struct InternalNode {
        int cnt = 0; // 表示分隔符个数，child个数是cnt + 1
        Key sepKeys[M];
        Value sepValues[M]; // 都是分隔符，范围为0 - cnt-1

        // >= 0 内部节点编号；< -1 表示叶子块号的编码（-2对应0）；== -1 表示空
        int child[M + 2]; // 0 - cnt

        InternalNode() {
            for (int i = 0; i < M + 2; i++) child[i] = -1;
        }

        void clear() {
            cnt = 0;
            for (int i = 0; i < M + 2; i++) child[i] = -1;
        }
    };

    // 块号（文件里的位置）到编码（child数组中编码）
    static int makeLeafChild (int leafBlockId) {
        return -(leafBlockId + 2);
    }
    // 编码到块号
    static int getLeafBlock (int childId) {
        return -(childId + 2);
    }
    static bool isLeaf(int childId) {
        return childId < -1;
    }

    struct Header {
        int leafBlockCount = 1;
        int freeLeafHead = -1;
        int firstLeafBlock = 0;
        int lastLeafBlock = 0;
    };

    struct Path {
        int nodeId; // 表示该节点为 pool[nodeId]
        int childPos; // 表示走 child[childPos]
    };

    std::fstream f;
    InternalNode *pool = nullptr;
    int internalNodeCount = 0;
    int root = makeLeafChild(0);
    Header header;
    std::string indexFileName; // 记录内部节点pool的数据
    Path path[100]; // 记录单次操作走的路径
    int pathDepth = 0; // 经过n个内部节点才到叶结点，一共有n+1层

    // 叶子IO
    void readLeaf(int blockId, Leaf &leaf) {
        f.seekg(sizeof(Header) + static_cast<long long>(blockId) * sizeof(Leaf));
        f.read(reinterpret_cast<char*>(&leaf), sizeof(Leaf));
    }
    void writeLeaf(int blockId, const Leaf &leaf) {
        f.seekp(sizeof(Header) + static_cast<long long>(blockId) * sizeof(Leaf));
        f.write(reinterpret_cast<const char*>(&leaf), sizeof(Leaf));
    }
    void readHeader() {
        f.seekg(0);
        f.read(reinterpret_cast<char*>(&header), sizeof(Header));
    }
    void writeHeader() {
        f.seekp(0);
        f.write(reinterpret_cast<const char*>(&header), sizeof(Header));
    }
    void freeLeaf(int blockId) {
        Leaf temp{};
        temp.next = header.freeLeafHead;
        writeLeaf(blockId, temp);
        header.freeLeafHead = blockId;
    }
    int newLeaf() {
        if (header.freeLeafHead != -1) {
            Leaf temp;
            int blockId = header.freeLeafHead;
            readLeaf(blockId, temp);
            header.freeLeafHead = temp.next;
            return blockId;
        }
        int blockId = header.leafBlockCount++;
        Leaf temp{};
        writeLeaf(blockId, temp);
        return blockId;
    }

    // compare
    // 1 < 2: true
    bool compareLess(const Key &k1, const Value &v1, const Key &k2, const Value &v2) {
        if (k1 < k2) return true;
        if (k2 < k1) return false;
        return v1 < v2;
    }
    bool compareEqual(const Key &k1, const Value &v1, const Key &k2, const Value &v2) {
        return (k1 == k2 && v1 == v2);
    }

    // 二分查找叶子内首个 >= (key, value) 的位置
    int lowerBound(const Leaf &leaf, const Key &key, const Value &value) {
        int low = 0, high = leaf.cnt;
        // [low, high)
        while (low < high) {
            int mid = low + (high - low) / 2;
            if (compareLess(leaf.k[mid], leaf.v[mid], key, value)) low = mid + 1;
            else high = mid;
        }
        return low;
    }

    // 帮助定位的函数
    // find辅助，找到可能含有key（不一定有）的最左边叶子节点
    int locateFirstLeaf(const Key &k) {
        int cur = root;
        while (!isLeaf(cur)) {
            InternalNode &node = pool[cur];
            int pos = 0;
            while (pos < node.cnt && node.sepKeys[pos] < k) pos++;
            cur = node.child[pos];
        }
        return getLeafBlock(cur);
    }
    // insert / erase 辅助，并且填充path
    int locateLeaf(const Key &k, const Value &v) {
        pathDepth = 0;
        int cur = root;
        while (!isLeaf(cur)) {
            InternalNode &node = pool[cur];
            int pos = 0;
            // 找第一个pos，(key, value) < sep[pos]
            while (pos < node.cnt && !compareLess(k, v, node.sepKeys[pos], node.sepValues[pos])) pos++;
            path[pathDepth++] = {cur, pos};
            cur = node.child[pos];
        }
        return getLeafBlock(cur);
    }


    // 把内部节点pool存到.idx文件里
    void saveIndex() {
        std::fstream file(indexFileName, std::ios::binary | std::ios::out);
        if (!file.is_open()) return;
        file.write(reinterpret_cast<const char *>(&internalNodeCount), sizeof(internalNodeCount));
        file.write(reinterpret_cast<const char *>(&root), sizeof(root));
        file.write(reinterpret_cast<const char *>(pool), sizeof(InternalNode) * internalNodeCount);
    }
    // 程序新开始时读取.idx中的pool信息
    void loadIndex() {
        std::fstream file(indexFileName, std::ios::binary | std::ios::in);
        if (!file.is_open()) {
            // 第一次运行，初始化
            internalNodeCount = 0;
            root = makeLeafChild(0);
            header = Header{};
            writeHeader();
            Leaf firstLeaf{};
            writeLeaf(0, firstLeaf);
            return;
        }
        file.read(reinterpret_cast<char *>(&internalNodeCount), sizeof(internalNodeCount));
        file.read(reinterpret_cast<char *>(&root), sizeof(root));
        if (internalNodeCount > 0) {
            file.read(reinterpret_cast<char *>(pool), sizeof(InternalNode) * internalNodeCount);
        }
        readHeader();
    }

    // insert辅助函数
    // 叶子节点满了，分裂成两个。把新分隔符和新右孩子推入父节点。rightChild：child数组中的值。这个函数不负责把叶子节点一分为二（写入文件）
    void pushUp(const Key &sepKey, const Value &sepValue, int rightChild) {
        if (pathDepth == 0) {
            // 原来的根是叶子节点，新建根
            int newRoot = internalNodeCount++;
            pool[newRoot].cnt = 1;
            pool[newRoot].sepKeys[0] = sepKey;
            pool[newRoot].sepValues[0] = sepValue;
            pool[newRoot].child[0] = root;
            pool[newRoot].child[1] = rightChild;
            root = newRoot;
            return;
        }
        Path &p = path[--pathDepth];
        InternalNode &node = pool[p.nodeId]; // 父节点
        int childPos = p.childPos;
        // 后面的都后移
        for (int i = node.cnt; i > childPos; --i) {
            node.sepKeys[i] = node.sepKeys[i - 1];
            node.sepValues[i] = node.sepValues[i - 1];
        }
        for (int i = node.cnt + 1; i > childPos + 1; --i) {
            node.child[i] = node.child[i - 1];
        }

        node.sepKeys[childPos] = sepKey;
        node.sepValues[childPos] = sepValue;
        node.child[childPos + 1] = rightChild;
        ++node.cnt;
        if (node.cnt < M) return;
        splitNode(p.nodeId);
    }

    // 内部节点（pool[nodeIndex]）溢出分裂（共m个分隔符）
    void splitNode(int nodeIndex) {
        InternalNode &left = pool[nodeIndex];
        int mid = (M - 1) / 2; // 中间的要上推，不保留在左右任何一边
        Key upKey = left.sepKeys[mid];
        Value upValue = left.sepValues[mid];

        int rightId = internalNodeCount++;
        InternalNode &right = pool[rightId];
        right.cnt = left.cnt - mid - 1;
        // copy to right
        for (int i = 0; i < right.cnt; ++i) {
            right.sepKeys[i] = left.sepKeys[i + mid + 1];
            right.sepValues[i] = left.sepValues[i + mid + 1];
        }
        // child[mid]在left中
        for (int i = 0; i <= right.cnt; ++i) {
            right.child[i] = left.child[i + mid + 1];
        }

        left.cnt = mid;
        pushUp(upKey, upValue, rightId);
    }

    // 叶子节点溢出分裂，blockId: 原来满的块，新的左半块
    void splitLeaf(int blockId, Leaf &leaf) {
        int leftCnt = (M + 1) / 2;
        int rightCnt = (M + 1) - leftCnt;
        int rightBlockId = newLeaf();
        Leaf rightLeaf{};
        rightLeaf.cnt = rightCnt;
        rightLeaf.prev = blockId;
        rightLeaf.next = leaf.next;
        for (int i = 0; i < rightCnt; ++i) {
            rightLeaf.k[i] = leaf.k[i + leftCnt];
            rightLeaf.v[i] = leaf.v[i + leftCnt];
        }
        leaf.cnt = leftCnt;
        leaf.next = rightBlockId;

        if (rightLeaf.next != -1) {
            Leaf nextLeaf;
            readLeaf(rightLeaf.next, nextLeaf);
            nextLeaf.prev = rightBlockId;
            writeLeaf(rightLeaf.next, nextLeaf);
        } else {
            header.lastLeafBlock = rightBlockId;
        }
        writeLeaf(blockId, leaf);
        writeLeaf(rightBlockId, rightLeaf);
        writeHeader();

        pushUp(rightLeaf.k[0], rightLeaf.v[0], makeLeafChild(rightBlockId));
    }


    // erase辅助函数
    // 在内部节点nodeId中删除分隔符(sep[i])和其右孩子(child[i + 1]). parentPos: nodeId父节点在path[]中的下标
    void removeSeparate(int nodeId, int sepPos, int parPos) {
        InternalNode &node = pool[nodeId];
        for (int i = sepPos; i < node.cnt - 1; ++i) {
            node.sepKeys[i] = node.sepKeys[i + 1];
            node.sepValues[i] = node.sepValues[i + 1];
        }
        for (int i = sepPos + 1; i < node.cnt; ++i) {
            node.child[i] = node.child[i + 1];
        }
        --node.cnt;

        if (parPos < 0) {
            if (node.cnt == 0) {
                // 本身就为根节点且只有一个sep，被删掉后无sep，唯一剩余的孩子变成根
                root = node.child[0];
            }
            return;
        }

        if (node.cnt >= MIN) return;
        fixNode(nodeId, parPos);
    }

    void fixNode(int nodeId, int parPos) {
        InternalNode &node = pool[nodeId];
        Path &p = path[parPos];
        InternalNode &parent = pool[p.nodeId];
        int childId = p.childPos; // 表示path走的是child[childId]

        // try to borrow from right
        if (childId < parent.cnt) {
            InternalNode &right = pool[parent.child[childId + 1]];
            if (right.cnt > MIN) {
                // 借成功了
                node.sepKeys[node.cnt] = parent.sepKeys[childId];
                node.sepValues[node.cnt] = parent.sepValues[childId];
                node.child[node.cnt + 1] = right.child[0];
                ++node.cnt;
                parent.sepKeys[childId] = right.sepKeys[0];
                parent.sepValues[childId] = right.sepValues[0];
                for (int i = 0; i < right.cnt - 1; ++i) {
                    right.sepKeys[i] = right.sepKeys[i + 1];
                    right.sepValues[i] = right.sepValues[i + 1];
                }
                for (int i = 0; i < right.cnt; ++i) {
                    right.child[i] = right.child[i + 1];
                }
                --right.cnt;
                return;
            }
        }
        // try to borrow from left
        if (childId > 0) {
            InternalNode &left = pool[parent.child[childId - 1]];
            if (left.cnt > MIN) {
                // 借成功了
                for (int i = node.cnt - 1; i >= 0; --i) {
                    node.sepKeys[i + 1] = node.sepKeys[i];
                    node.sepValues[i + 1] = node.sepValues[i];
                }
                for (int i = node.cnt; i >= 0; --i) {
                    node.child[i + 1] = node.child[i];
                }
                node.sepKeys[0] = parent.sepKeys[childId - 1];
                node.sepValues[0] = parent.sepValues[childId - 1];
                node.child[0] = left.child[left.cnt];
                ++node.cnt;
                parent.sepKeys[childId - 1] = left.sepKeys[left.cnt - 1];
                parent.sepValues[childId - 1] = left.sepValues[left.cnt - 1];
                --left.cnt;
                return;
            }
        }

        // merge with right
        if (childId < parent.cnt) {
            InternalNode &right = pool[parent.child[childId + 1]];
            node.sepKeys[node.cnt] = parent.sepKeys[childId];
            node.sepValues[node.cnt] = parent.sepValues[childId];
            for (int i = 0; i < right.cnt; ++i) {
                node.sepKeys[i + 1 + node.cnt] = right.sepKeys[i];
                node.sepValues[i + 1 + node.cnt] = right.sepValues[i];
                node.child[i + 1 + node.cnt] = right.child[i];
            }
            node.child[node.cnt + 1 + right.cnt] = right.child[right.cnt];
            node.cnt += (1 + right.cnt);
            // remove sep[childId] and right
            removeSeparate(p.nodeId, childId, parPos - 1);
        }
        // merge with left
        else {
            InternalNode &left = pool[parent.child[childId - 1]];
            left.sepKeys[left.cnt] = parent.sepKeys[childId - 1];
            left.sepValues[left.cnt] = parent.sepValues[childId - 1];
            for (int i = 0; i < node.cnt; ++i) {
                left.sepKeys[left.cnt + 1 + i] = node.sepKeys[i];
                left.sepValues[left.cnt + 1 + i] = node.sepValues[i];
                left.child[left.cnt + 1 + i] = node.child[i];
            }
            left.child[left.cnt + 1 + node.cnt] = node.child[node.cnt];
            left.cnt += (1 + node.cnt);
            // remove sep[childId - 1] and node
            removeSeparate(p.nodeId, childId - 1, parPos - 1);
        }
    }

    void fixLeaf(int blockId, Leaf &leaf) {
        if (pathDepth == 0) return;
        Path &p = path[pathDepth - 1];
        InternalNode &parent = pool[p.nodeId];
        int childId = p.childPos;

        // try to borrow from right
        if (childId < parent.cnt) {
            int rightId = getLeafBlock(parent.child[childId + 1]);
            Leaf right;
            readLeaf(rightId, right);
            if (right.cnt > MIN) {
                leaf.k[leaf.cnt] = right.k[0];
                leaf.v[leaf.cnt] = right.v[0];
                ++leaf.cnt;
                for (int i = 0; i < right.cnt - 1; ++i) {
                    right.k[i] = right.k[i + 1];
                    right.v[i] = right.v[i + 1];
                }
                --right.cnt;
                parent.sepKeys[childId] = right.k[0];
                parent.sepValues[childId] = right.v[0];
                writeLeaf(rightId, right);
                writeLeaf(blockId, leaf);
                return;
            }
        }
        // try to borrow from left
        if (childId > 0) {
            int leftId = getLeafBlock(parent.child[childId - 1]);
            Leaf left;
            readLeaf(leftId, left);
            if (left.cnt > MIN) {
                for (int i = leaf.cnt; i > 0; --i) {
                    leaf.k[i] = leaf.k[i - 1];
                    leaf.v[i] = leaf.v[i - 1];
                }
                leaf.k[0] = left.k[left.cnt - 1];
                leaf.v[0] = left.v[left.cnt - 1];
                ++leaf.cnt;
                --left.cnt;
                parent.sepKeys[childId - 1] = leaf.k[0];
                parent.sepValues[childId - 1] = leaf.v[0];
                writeLeaf(leftId, left);
                writeLeaf(blockId, leaf);
                return;
            }
        }
        // merge with right
        if (childId < parent.cnt) {
            int rightId = getLeafBlock(parent.child[childId + 1]);
            Leaf right;
            readLeaf(rightId, right);
            for (int i = 0; i < right.cnt; ++i) {
                leaf.k[i + leaf.cnt] = right.k[i];
                leaf.v[i + leaf.cnt] = right.v[i];
            }
            leaf.cnt += right.cnt;
            leaf.next = right.next;
            if (right.next != -1) {
                Leaf next;
                readLeaf(right.next, next);
                next.prev = blockId;
                writeLeaf(right.next, next);
            } else {
                header.lastLeafBlock = blockId;
            }
            writeLeaf(blockId, leaf);
            freeLeaf(rightId);
            writeHeader();
            removeSeparate(p.nodeId, childId, pathDepth - 2);
        }
        // merge with left
        else {
            int leftId = getLeafBlock(parent.child[childId - 1]);
            Leaf left;
            readLeaf(leftId, left);
            for (int i = 0; i < leaf.cnt; ++i) {
                left.k[i + left.cnt] = leaf.k[i];
                left.v[i + left.cnt] = leaf.v[i];
            }
            left.cnt += leaf.cnt;
            left.next = leaf.next;
            if (leaf.next != -1) {
                Leaf next;
                readLeaf(leaf.next, next);
                next.prev = leftId;
                writeLeaf(leaf.next, next);
            } else {
                header.lastLeafBlock = leftId;
            }
            writeLeaf(leftId, left);
            freeLeaf(blockId);
            writeHeader();
            removeSeparate(p.nodeId, childId - 1, pathDepth - 2);
        }
    }


public:
    BPlusTree(const std::string &dataFileName) : indexFileName(dataFileName + ".idx") {
        // dataFileName是存叶子结点的，indexFileName是pool
        pool = new InternalNode[MAX_INTERNAL_NODES]();
        f.open(dataFileName, std::ios::binary | std::ios::in | std::ios::out);
        if (!f.is_open()) {
            f.open(dataFileName, std::ios::binary | std::ios::out);
            f.close();
            f.open(dataFileName, std::ios::binary | std::ios::in | std::ios::out);
        }
        loadIndex();
    }

    ~BPlusTree() {
        saveIndex();
        delete[] pool;
        f.close();
    }

    void find(const Key &key) {
        int blockId = locateFirstLeaf(key);
        bool found = false;
        while (blockId != -1) {
            Leaf leaf;
            readLeaf(blockId, leaf);
            int i = 0;
            while (i < leaf.cnt && leaf.k[i] < key) ++i;
            while (i < leaf.cnt) {
                if (key < leaf.k[i]) {
                    // 超过key了
                    if (!found) std::cout << "null";
                    std::cout << '\n';
                    return;
                }
                if (found) std::cout << " ";
                std::cout << leaf.v[i];
                found = true;
                ++i;
            }
            blockId = leaf.next;
        }
        if (!found) std::cout << "null";
        std::cout << '\n';
    }

    void insert(const Key &key, const Value &value) {
        int blockId = locateLeaf(key, value);
        Leaf leaf;
        readLeaf(blockId, leaf);

        int pos = lowerBound(leaf, key, value);
        if (pos < leaf.cnt && compareEqual(leaf.k[pos], leaf.v[pos], key, value)) {
            // 重复了，不插入
            return;
        }

        for (int i = leaf.cnt; i > pos; --i) {
            leaf.k[i] = leaf.k[i - 1];
            leaf.v[i] = leaf.v[i - 1];
        }
        leaf.k[pos] = key;
        leaf.v[pos] = value;
        ++leaf.cnt;
        if (leaf.cnt <= M) {
            writeLeaf(blockId, leaf);
            return;
        }
        splitLeaf(blockId, leaf);
    }

    void erase(const Key &key, const Value &value) {
        int blockId = locateLeaf(key, value);
        Leaf leaf;
        readLeaf(blockId, leaf);

        int pos = lowerBound(leaf, key, value);
        if (pos >= leaf.cnt || !compareEqual(leaf.k[pos], leaf.v[pos], key, value)) {
            // 不存在
            return;
        }

        for (int i = pos; i < leaf.cnt - 1; ++i) {
            leaf.k[i] = leaf.k[i + 1];
            leaf.v[i] = leaf.v[i + 1];
        }
        --leaf.cnt;
        writeLeaf(blockId, leaf);

        if (pathDepth == 0 || leaf.cnt >= MIN) return;
        fixLeaf(blockId, leaf);
    }
};


