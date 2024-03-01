#ifndef PTREENODE_HPP
#define PTREENODE_HPP

#include "Commons.hpp"
#include "Logger.hpp"

#ifdef ENCLAVE
#else
#include "ocalls.hpp"
#endif

class PTreeNode {
private:
    tuple_t *start;
    uint64_t num = 0;
    const uint32_t MASK;
    const uint32_t TOTAL_BITS; // the number of total bits the join looks at. it is equal to numbits in the join
    const uint32_t BITS;
    PTreeNode *branch0 = nullptr;
    PTreeNode *branch1 = nullptr;

    PTreeNode *getNodeByMask(PTreeNode *pNode, int mask, int bits) {
        if (bits < 1) return pNode;

        int bit = Commons::check_bit(mask, bits - 1);

        if (bit == 1 && pNode->getBranch1()) {
            pNode = getNodeByMask(pNode->getBranch1(), mask, bits - 1);
        } else if (bit == 0 && pNode->getBranch0()) {
            pNode = getNodeByMask(pNode->getBranch0(), mask, bits - 1);
        }

        return pNode;
    }

public:
    PTreeNode(tuple_t *_start, uint64_t _num, uint32_t _MASK, uint32_t _TOTAL_BITS, uint32_t _BITS) :
            start(_start), num(_num), MASK(_MASK), TOTAL_BITS(_TOTAL_BITS), BITS(_BITS) {}

    PTreeNode(relation_t *_rel, uint32_t _TOTAL_BITS) :
            start(_rel->tuples), num(_rel->num_tuples), MASK(0), TOTAL_BITS(_TOTAL_BITS), BITS(0) {}

    [[nodiscard]] PTreeNode *
    getBranch0() const {
        return branch0;
    }

    void
    setBranch0(PTreeNode *pNode) {
        //SGX_ASSERT(pNode != nullptr, "Can not set branch0 to NULL");
        PTreeNode::branch0 = pNode;
    }

    [[nodiscard]] PTreeNode *
    getBranch1() const {
        return branch1;
    }

    void
    setBranch1(PTreeNode *pNode) {
        //SGX_ASSERT(pNode != nullptr, "Can not set branch1 to NULL");
        PTreeNode::branch1 = pNode;
    }

    [[nodiscard]] uint32_t
    getMask() const {
        return MASK;
    }

    [[nodiscard]] uint32_t
    getBits() const {
        return BITS;
    }

    [[nodiscard]] tuple_t *
    getStart() const {
        return start;
    }

    [[nodiscard]] uint64_t
    getNum() const {
        return num;
    }

    [[nodiscard]] uint32_t
    getTotalBits() const {
        return TOTAL_BITS;
    }

    /**
     * Inserts children for existing node identified by parentMask and bits.
     * @param start0
     * @param num0
     * @param start1
     * @param num1
     * @param parentMask
     * @param bits
     */
    void
    insert(tuple_t *start0, uint64_t num0, tuple_t *start1, uint64_t num1, int parentMask, int bits) {
//        logger(DBG, "PTree insert: num0=%u, num1=%u, parentMask=%d, bits=%d", num0, num1, parentMask, bits);
        PTreeNode *pNode = getNodeByMask(this, parentMask, bits);
        int mask = parentMask << 1;
        pNode->setBranch0(new PTreeNode(start0, num0, mask, TOTAL_BITS, bits + 1));
        mask = Commons::set_bit(mask, 0);
        pNode->setBranch1(new PTreeNode(start1, num1, mask, TOTAL_BITS, bits + 1));

    }

    ~PTreeNode() {
        if (branch0) delete branch0;
        if (branch1) delete branch1;
    }

    PTreeNode *
    getSlice(PTreeNode *pNode, uint32_t mask, int bits) {
        int bit = Commons::check_bit(mask, (bits - 1));
        if (bit) {
            if (pNode->branch1) {
                pNode = getSlice(pNode->branch1, mask, (bits - 1));
            } else if (pNode->branch0) {
                throw "Trying to return a non-leaf slice";
            }
        } else {
            if (pNode->branch0) {
                pNode = getSlice(pNode->branch0, mask, (bits - 1));
            } else if (pNode->branch1) {
                throw "Trying to return a non-leaf slice";
            }
        }
        return pNode;
    }

    void
    traverse(PTreeNode *pNode) {
        if (pNode) {
            logger(INFO, "0x%08x|%d  -> start=%p, num=%lu, bits=%d",
                   pNode->MASK, pNode->BITS, pNode->start, pNode->num, pNode->BITS);
            traverse(pNode->branch0);
            traverse(pNode->branch1);
        }
    }
};

#endif //PTREENODE_HPP
