// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // RBTree

    template <typename T, RBTreeLink<T> T::* Link, typename K, K T::* Key>
    RBTree<T, Link, K, Key>& RBTree<T, Link, K, Key>::operator=(RBTree&& x)
    {
        // Has to already be empty because this class doesn't actually know how to destroy nodes
        HE_ASSERT(IsEmpty());
        m_root = Exchange(x.m_root, nullptr);
        return *this;
    }

    template <typename T, RBTreeLink<T> T::* Link, typename K, K T::* Key>
    T* RBTree<T, Link, K, Key>::Next(T* node) const
    {
        if (IsEmpty() || !node)
            return nullptr;

        T* ret = nullptr;
        if (T* right = Right(node); right != nullptr)
        {
            ret = First(right);
        }
        else
        {
            T* tnode = m_root;
            ret = nullptr;
            while (true)
            {
                if ((node->*Key) < (tnode->*Key))
                {
                    ret = tnode;
                    tnode = Left(tnode);
                }
                else if ((node->*Key) == (tnode->*Key))
                {
                    break;
                }
                else // greater-than
                {
                    tnode = Right(tnode);
                }
                HE_ASSERT(tnode != nullptr);
            }
        }
        return ret;
    }

    template <typename T, RBTreeLink<T> T::* Link, typename K, K T::* Key>
    T* RBTree<T, Link, K, Key>::Prev(T* node) const
    {
        if (IsEmpty() || !node)
            return nullptr;

        T* ret = nullptr;
        if (T* left = Left(node); left != nullptr)
        {
            ret = Last(left);
        }
        else
        {
            T* tnode = m_root;
            ret = nullptr;
            while (true)
            {
                if ((node->*Key) < (tnode->*Key))
                {
                    tnode = Left(tnode);
                }
                else if ((node->*Key) == (tnode->*Key))
                {
                    break;
                }
                else // greater-than
                {
                    ret = tnode;
                    tnode = Right(tnode);
                }
                HE_ASSERT(tnode != nullptr);
            }
        }
        return ret;
    }

    template <typename T, RBTreeLink<T> T::* Link, typename K, K T::* Key>
    void RBTree<T, Link, K, Key>::Clear(Pfn_ClearHandler handler, void* userData)
    {
        ClearInternal(m_root, handler, userData);
        m_root = nullptr;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::Find(const K& key) const
    {
        if (!m_root)
            return nullptr;

        T* ret = m_root;
        while (ret && key != (ret->*Key))
        {
            if (key < (ret->*Key))
                ret = Left(ret);
            else
                ret = Right(ret);
        }

        return ret;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::Insert(T* node)
    {
        if (!node)
            return;

        PathEntry path[MaxDepth];
        PathEntry* pathp = nullptr;

        Init(node);

        path->node = m_root;
        for (pathp = path; pathp->node != nullptr; ++pathp)
        {
            int cmp = pathp->cmp = (node->*Key) == ((pathp->node)->*Key) ? 0 : ((node->*Key) < ((pathp->node)->*Key) ? -1 : 1);
            HE_ASSERT(cmp != 0);

            if (cmp < 0)
                pathp[1].node = Left(pathp->node);
            else
                pathp[1].node = Right(pathp->node);
        }

        pathp->node = node;
        HE_ASSERT(Left(node) == nullptr);
        HE_ASSERT(Right(node) == nullptr);

        for (pathp--; pathp >= path; --pathp)
        {
            T* cnode = pathp->node;
            if (pathp->cmp < 0)
            {
                T* left = pathp[1].node;
                SetLeft(cnode, left);
                if (IsRed(left))
                {
                    T* leftLeft = Left(left);
                    if (leftLeft && IsRed(leftLeft))
                    {
                        SetBlack(leftLeft);
                        cnode = RotateRight(cnode);
                    }
                }
                else
                {
                    return;
                }
            }
            else
            {
                T* right = pathp[1].node;
                SetRight(cnode, right);
                if (IsRed(right))
                {
                    T* left = Left(cnode);
                    if (left && IsRed(left))
                    {
                        SetBlack(left);
                        SetBlack(right);
                        SetRed(cnode);
                    }
                    else
                    {
                        const bool tred = IsRed(cnode);
                        T* tnode = RotateLeft(cnode);
                        SetColor(tnode, tred);
                        SetRed(cnode);
                        cnode = tnode;
                    }
                }
                else
                {
                    return;
                }
            }
            pathp->node = cnode;
        }
        m_root = path->node;
        SetBlack(m_root);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::Remove(const K& key)
    {
        PathEntry path[MaxDepth];
        PathEntry* pathp = nullptr;
        PathEntry* nodep = nullptr;
        PathEntry* swapp = nullptr;

        path[0].node = m_root;
        for (pathp = path; pathp->node != nullptr; ++pathp)
        {
            int cmp = pathp->cmp = key == ((pathp->node)->*Key) ? 0 : (key < ((pathp->node)->*Key) ? -1 : 1);
            if (cmp < 0)
            {
                pathp[1].node = Left(pathp->node);
            }
            else
            {
                pathp[1].node = Right(pathp->node);

                if (cmp == 0)
                {
                    pathp->cmp = 1;
                    nodep = pathp;
                    for (++pathp; pathp->node != nullptr; ++pathp)
                    {
                        pathp->cmp = -1;
                        pathp[1].node = Left(pathp->node);
                    }
                    break;
                }
            }
        }

        if (!nodep || !nodep->node)
            return nullptr;

        T* const node = nodep->node;
        --pathp;

        if (pathp->node != node)
        {
            swapp = nodep;
            const bool tred = IsRed(pathp->node);
            SetColor(pathp->node, IsRed(node));
            SetLeft(pathp->node, Left(node));
            SetRight(pathp->node, Right(node));
            SetColor(node, tred);

            nodep->node = pathp->node;
            pathp->node = node;
            if (nodep == path)
            {
                m_root = nodep->node;
            }
            else
            {
                if (nodep[-1].cmp < 0)
                    SetLeft(nodep[-1].node, nodep->node);
                else
                    SetRight(nodep[-1].node, nodep->node);
            }
        }
        else
        {
            T* left = Left(node);
            if (left)
            {
                HE_ASSERT(!IsRed(node));
                HE_ASSERT(IsRed(left));
                SetBlack(left);
                if (pathp == path)
                {
                    m_root = left;
                }
                else
                {
                    if (pathp[-1].cmp < 0)
                        SetLeft(pathp[-1].node, left);
                    else
                        SetRight(pathp[-1].node, left);
                }
                return node;
            }
            else if (pathp == path)
            {
                m_root = nullptr;
                return node;
            }
        }

        if (IsRed(pathp->node))
        {
            HE_ASSERT(pathp[-1].cmp < 0);
            SetLeft(pathp[-1].node, nullptr);
            return node;
        }

        pathp->node = nullptr;
        for (--pathp; pathp >= path; --pathp)
        {
            HE_ASSERT(pathp->cmp != 0);
            if (pathp->cmp < 0)
            {
                SetLeft(pathp->node, pathp[1].node);
                if (IsRed(pathp->node))
                {
                    T* right = Right(pathp->node);
                    T* rightLeft = Left(right);
                    T* tnode = nullptr;

                    if (rightLeft && IsRed(rightLeft))
                    {
                        // In the following diagrams, ||, //, and \\
                        // indicate the path to the removed node.
                        //
                        //      ||
                        //    pathp(r)
                        //  //        \
                        // (b)        (b)
                        //           /
                        //          (r)
                        SetBlack(pathp->node);
                        tnode = RotateRight(right);
                        SetRight(pathp->node, tnode);
                        tnode = RotateLeft(pathp->node);
                    }
                    else
                    {
                        //      ||
                        //    pathp(r)
                        //  //        \
                        // (b)        (b)
                        //           /
                        //          (b)
                        tnode = RotateLeft(pathp->node);
                    }
                    HE_ASSERT(pathp > path);
                    if (pathp[-1].cmp < 0)
                        SetLeft(pathp[-1].node, tnode);
                    else
                        SetRight(pathp[-1].node, tnode);
                    return node;
                }
                else
                {
                    T* right = Right(pathp->node);
                    T* rightLeft = Left(right);

                    if (rightLeft && IsRed(rightLeft))
                    {
                        //      ||
                        //    pathp(b)
                        //  //        \
                        // (b)        (b)
                        //           /
                        //          (r)
                        T* tnode = nullptr;
                        SetBlack(rightLeft);
                        tnode = RotateRight(right);
                        SetRight(pathp->node, tnode);
                        tnode = RotateLeft(pathp->node);

                        if (pathp == path)
                        {
                            m_root = tnode;
                        }
                        else
                        {
                            if (pathp[-1].cmp < 0)
                                SetLeft(pathp[-1].node, tnode);
                            else
                                SetRight(pathp[-1].node, tnode);
                        }
                        return node;
                    }
                    else
                    {
                        //      ||
                        //    pathp(b)
                        //  //        \
                        // (b)        (b)
                        //           /
                        //          (b)
                        T* tnode = nullptr;
                        SetRed(pathp->node);
                        tnode = RotateLeft(pathp->node);
                        pathp->node = tnode;
                    }
                }
            }
            else
            {
                SetRight(pathp->node, pathp[1].node);
                T* left = Left(pathp->node);
                if (IsRed(left))
                {
                    T* leftRight = Right(left);
                    T* leftRightLeft = Left(leftRight);
                    T* tnode = nullptr;

                    if (leftRightLeft && IsRed(leftRightLeft))
                    {
                        //      ||
                        //    pathp(b)
                        //   /        \\
                        // (r)        (b)
                        //   \
                        //   (b)
                        //   /
                        // (r)
                        T* unode;
                        SetBlack(leftRightLeft);
                        unode = RotateRight(pathp->node);
                        tnode = RotateRight(pathp->node);
                        SetRight(unode, tnode);
                        tnode = RotateLeft(unode);
                    }
                    else
                    {
                        //      ||
                        //    pathp(b)
                        //   /        \\
                        // (r)        (b)
                        //   \
                        //   (b)
                        //   /
                        // (b)
                        HE_ASSERT(leftRight != nullptr);
                        SetRed(leftRight);
                        tnode = RotateRight(pathp->node);
                        SetBlack(tnode);
                    }

                    if (pathp == path)
                    {
                        m_root = tnode;
                    }
                    else
                    {
                        if (pathp[-1].cmp < 0)
                            SetLeft(pathp[-1].node, tnode);
                        else
                            SetRight(pathp[-1].node, tnode);
                    }
                    return node;
                }
                else if (IsRed(pathp->node))
                {
                    T* leftLeft = Left(left);

                    if (leftLeft && IsRed(leftLeft))
                    {
                        //        ||
                        //      pathp(r)
                        //     /        \\
                        //   (b)        (b)
                        //   /
                        // (r)
                        SetBlack(pathp->node);
                        SetRed(left);
                        SetBlack(leftLeft);
                        T* tnode = RotateRight(pathp->node);

                        HE_ASSERT(pathp > path);
                        if (pathp[-1].cmp < 0)
                            SetLeft(pathp[-1].node, tnode);
                        else
                            SetRight(pathp[-1].node, tnode);
                        return node;
                    }
                    else
                    {
                        //        ||
                        //      pathp(r)
                        //     /        \\
                        //   (b)        (b)
                        //   /
                        // (b)
                        SetRed(left);
                        SetBlack(pathp->node);
                        return node;
                    }
                }
                else
                {
                    T* leftLeft = Left(left);

                    if (leftLeft && IsRed(leftLeft))
                    {
                        //        ||
                        //      pathp(b)
                        //     /        \\
                        //   (b)        (b)
                        //   /
                        // (r)
                        SetBlack(leftLeft);
                        T* tnode = RotateRight(pathp->node);

                        if (pathp == path)
                        {
                            m_root = tnode;
                        }
                        else
                        {
                            if (pathp[-1].cmp < 0)
                                SetLeft(pathp[-1].node, tnode);
                            else
                                SetRight(pathp[-1].node, tnode);
                        }
                        return node;
                    }
                    else
                    {
                        //        ||
                        //      pathp(b)
                        //     /        \\
                        //   (b)        (b)
                        //   /
                        // (b)
                        SetRed(left);
                    }
                }
            }
        }
        m_root = path->node;
        HE_ASSERT(!IsRed(m_root));
        return node;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::CopyFrom(const RBTree& other, Pfn_AllocNodeCopy alloc, void* userData)
    {
        // Has to already be empty because this class doesn't actually know how to destroy nodes
        HE_ASSERT(IsEmpty());

        if (other.IsEmpty())
            return;

        m_root = alloc(other.m_root, userData);
        CopyNode(m_root, other.m_root, alloc, userData);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::ClearInternal(T* node, Pfn_ClearHandler handler, void* userData)
    {
        if (!node)
            return;

        ClearInternal(Left(node), handler, userData);
        SetLeft(node, nullptr);

        ClearInternal(Right(node), handler, userData);
        SetRight(node, nullptr);

        if (handler)
            handler(node, userData);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::CopyNode(T* to, T* from, Pfn_AllocNodeCopy alloc, void* userData)
    {
        T* left = Left(from);
        if (left)
        {
            T* newLeft = alloc(left, userData);
            SetLeft(to, newLeft);
            CopyNode(newLeft, left, alloc, userData);
        }
        else
        {
            SetLeft(to, nullptr);
        }

        T* right = Right(from);
        if (right)
        {
            T* newRight = alloc(right, userData);
            SetRight(to, newRight);
            CopyNode(newRight, right, alloc, userData);
        }
        else
        {
            SetRight(to, nullptr);
        }

        SetColor(to, IsRed(from));
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::Init(T* node)
    {
        HE_ASSERT((reinterpret_cast<uintptr_t>(node) & RedFlag) == 0);
        SetLeft(node, nullptr);
        SetRight(node, nullptr);
        SetRed(node);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::Left(const T* node)
    {
        return (node->*Link).left;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::SetLeft(T* node, T* left)
    {
        (node->*Link).left = left;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::Right(const T* node)
    {
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        return reinterpret_cast<T*>(value & RedMask);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::SetRight(T* node, T* right)
    {
        const uintptr_t newValue = reinterpret_cast<uintptr_t>(right);
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        (node->*Link).rightRed = reinterpret_cast<T*>(newValue | (value & RedFlag));
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    bool RBTree<T, Link, K, Key>::IsRed(const T* node)
    {
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        return static_cast<bool>(value & RedFlag);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::SetRed(T* node)
    {
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        (node->*Link).rightRed = reinterpret_cast<T*>(value | RedFlag);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::SetBlack(T* node)
    {
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        (node->*Link).rightRed = reinterpret_cast<T*>(value & RedMask);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    void RBTree<T, Link, K, Key>::SetColor(T* node, bool red)
    {
        const uintptr_t value = reinterpret_cast<uintptr_t>((node->*Link).rightRed);
        const uintptr_t redFlag = static_cast<uintptr_t>(red);
        (node->*Link).rightRed = reinterpret_cast<T*>((value & RedMask) | redFlag);
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::RotateLeft(T* node)
    {
        T* ret = Right(node);
        SetRight(node, Left(ret));
        SetLeft(ret, node);
        return ret;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::RotateRight(T* node)
    {
        T* ret = Left(node);
        SetLeft(node, Right(ret));
        SetRight(ret, node);
        return ret;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::First(T* node)
    {
        while (node && Left(node) != nullptr)
        {
            node = Left(node);
        }
        return node;
    }

    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    T* RBTree<T, Link, K, Key>::Last(T* node)
    {
        while (node && Right(node) != nullptr)
        {
            node = Right(node);
        }
        return node;
    }

    // --------------------------------------------------------------------------------------------
    // RBTreeContainerBase

    template <typename T>
    RBTreeContainerBase<T>::RBTreeContainerBase(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {}

    template <typename T>
    RBTreeContainerBase<T>::RBTreeContainerBase(const RBTreeContainerBase& x, Allocator& allocator) noexcept
        : RBTreeContainerBase(allocator)
    {
        CopyFrom(x);
    }

    template <typename T>
    RBTreeContainerBase<T>::RBTreeContainerBase(RBTreeContainerBase&& x, Allocator& allocator) noexcept
        : RBTreeContainerBase(allocator)
    {
        MoveFrom(Move(x));
    }

    template <typename T>
    RBTreeContainerBase<T>::RBTreeContainerBase(const RBTreeContainerBase& x) noexcept
        : RBTreeContainerBase(x, x.m_allocator)
    {}

    template <typename T>
    RBTreeContainerBase<T>::RBTreeContainerBase(RBTreeContainerBase&& x) noexcept
        : RBTreeContainerBase(Move(x), x.m_allocator)
    {}

    template <typename T>
    RBTreeContainerBase<T>::~RBTreeContainerBase() noexcept
    {
        Clear();
    }

    template <typename T>
    RBTreeContainerBase<T>& RBTreeContainerBase<T>::operator=(const RBTreeContainerBase& x) noexcept
    {
        CopyFrom(x);
        return *this;
    }

    template <typename T>
    RBTreeContainerBase<T>& RBTreeContainerBase<T>::operator=(RBTreeContainerBase&& x) noexcept
    {
        MoveFrom(Move(x));
        return *this;
    }

    template <typename T>
    bool RBTreeContainerBase<T>::operator==(const RBTreeContainerBase& x) const
    {
        if (this == &x)
            return true;

        if (m_size != x.m_size)
            return false;

        ConstIterator itA = Begin();
        ConstIterator itB = x.Begin();
        for (; itA != End() && itB != x.End(); ++itA, ++itB)
        {
            if (!itA || !itB)
                return false;

            if (itA->key != itB->key || itA->value != itB->value)
                return false;
        }

        return true;
    }

    template <typename T>
    template <typename U>
    const typename RBTreeContainerBase<T>::EntryType* RBTreeContainerBase<T>::Find(const U& key) const
    {
        return m_tree.Find(key);
    }

    template <typename T>
    template <typename U>
    const typename RBTreeContainerBase<T>::EntryType& RBTreeContainerBase<T>::Get(const U& key) const
    {
        const EntryType* entry = Find(key);
        HE_ASSERT(entry);
        return *entry;
    }

    template <typename T>
    void RBTreeContainerBase<T>::Clear()
    {
        m_tree.Clear([](EntryType* entry, void* userData)
        {
            if (entry)
            {
                RBTreeContainerBase* self = static_cast<RBTreeContainerBase*>(userData);
                self->m_allocator.Delete(entry);
            }
        }, this);
        m_size = 0;
    }

    template <typename T>
    template <typename U>
    bool RBTreeContainerBase<T>::Erase(const U& key)
    {
        EntryType* entry = m_tree.Remove(key);
        if (!entry)
            return false;

        m_allocator.Delete(entry);
        --m_size;
        return true;
    }

    template <typename T>
    template <typename U, typename... Args>
    RBTreeContainerBase<T>::EmplaceResult RBTreeContainerBase<T>::Emplace(U&& key, Args&&... args)
    {
        EntryType* entry = m_tree.Find(key);
        if (entry)
            return { *entry, false };

        entry = m_allocator.New<EntryType>(Forward<U>(key), Forward<Args>(args)...);
        m_tree.Insert(entry);
        ++m_size;
        return { *entry, true };
    }

    template <typename T>
    void RBTreeContainerBase<T>::CopyFrom(const RBTreeContainerBase& x)
    {
        if (this == &x)
            return;

        Clear();

        m_tree.CopyFrom(x.m_tree, [](const EntryType* entry, void* userData)
        {
            RBTreeContainerBase* self = static_cast<RBTreeContainerBase*>(userData);
            return self->m_allocator.template New<EntryType>(*entry);
        }, this);
    }

    template <typename T>
    void RBTreeContainerBase<T>::MoveFrom(RBTreeContainerBase&& x)
    {
        if (this == &x)
            return;

        Clear();

        if (&m_allocator != &x.m_allocator)
        {
            CopyFrom(x);
            x.Clear();
            return;
        }

        m_tree = Move(x.m_tree);
    }

    // --------------------------------------------------------------------------------------------
    // RBTreeMap

    template <typename K, typename V>
    template <typename U, typename X>
    typename RBTreeMap<K, V>::EmplaceResult RBTreeMap<K, V>::EmplaceOrAssign(U&& key, X&& value)
    {
        const EmplaceResult result = Super::Emplace(Forward<U>(key), Forward<X>(value));
        if (!result.inserted)
        {
            result.entry.value = Forward<X>(value);
        }
        return result;
    }

    template <typename K, typename V>
    template <typename U>
    const typename RBTreeMap<K, V>::ValueType* RBTreeMap<K, V>::Find(const U& key) const
    {
        const EntryType* v = Super::Find(key);
        return v ? &v->value : nullptr;
    }

    template <typename K, typename V>
    template <typename U>
    typename RBTreeMap<K, V>::ValueType* RBTreeMap<K, V>::Find(const U& key)
    {
        EntryType* v = Super::Find(key);
        return v ? &v->value : nullptr;
    }
}
