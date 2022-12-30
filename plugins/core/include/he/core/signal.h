// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

namespace he
{
    template <typename>
    class Signal;

    template <typename R, typename... Args>
    class Signal<R(Args...)>
    {
    public:
        using DelegateType = Delegate<R(Args...)>;

        class Binding
        {
        public:
            Binding() = default;

            [[nodiscard]] bool IsValid() const { return m_signal != nullptr; }
            bool Detach() { return IsValid() && m_signal->Detach(*this); }

            [[nodiscard]] explicit operator bool() const { return IsValid(); }

        private:
            friend Signal;

            Binding(Signal* signal, uint64_t token)
                : m_signal(signal)
                , m_token(token)
            {}

            Signal* m_signal{ nullptr };
            uint64_t m_token{ 0 };
        };

    private:
        struct Attachment
        {
            DelegateType func;
            uint64_t token;
            bool once;
        };

    public:
        Signal(Allocator& allocator = Allocator::GetDefault())
            : m_attachments(allocator)
        {}

        [[nodiscard]] uint32_t Size() const { return m_attachments.Size(); }
        [[nodiscard]] bool IsEmpty() const { return m_attachments.IsEmpty(); }

        template <auto F, typename T>
        Binding Attach(T&& payload)
        {
            return AddAttachment(DelegateType::template Make<F>(Forward<T>(payload)), false);
        }

        Binding Attach(typename DelegateType::FunctionType* func, const void* payload = nullptr)
        {
            return AddAttachment(DelegateType::template Make(func, payload), false);
        }

        template <auto F, typename T>
        Binding AttachOnce(T&& payload)
        {
            return AddAttachment(DelegateType::template Make<F>(Forward<T>(payload)), true);
        }

        Binding AttachOnce(typename DelegateType::FunctionType* func, const void* payload = nullptr)
        {
            return AddAttachment(DelegateType::template Make(func, payload), true);
        }

        bool Detach(const Binding& binding)
        {
            if (binding.m_signal != this)
                return false;

            for (uint32_t i = 0; i < m_attachments.Size(); ++i)
            {
                if (m_attachments[i].token == binding.m_token)
                {
                    m_attachments.Erase(i, 1);
                    return true;
                }
            }

            return false;
        }

        void DetachAll()
        {
            m_attachments.Clear();
        }

        void Dispatch(Args... args)
        {
            for (uint32_t i = 0; i < m_attachments.Size();)
            {
                Attachment& a = m_attachments[i];
                a.func(Forward<Args>(args)...);
                if (a.once)
                    m_attachments.Erase(i, 1);
                else
                    ++i;
            }
        }

    private:
        Binding AddAttachment(DelegateType func, bool once)
        {
            const uint64_t token = m_nextToken++;

            m_attachments.PushBack({ func, token, once });
            return Binding(this, token);
        }

    private:
        Vector<Attachment> m_attachments;
        uint64_t m_nextToken{ 1 };
    };
}
