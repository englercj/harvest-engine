// Copyright Chad Engler

#include "he/schema/layout.h"

#include "he/core/error.h"
#include "he/core/test.h"

using namespace he::schema;

// ------------------------------------------------------------------------------------------------
alignas(8) static const uint8_t SimpleStructTestBytes[] =
{
    // Struct pointer, offset = 0, dataSize = 2, pointerCount = 0
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    // Struct metadata, fieldCount = 4, bitset = 1101
    0x04, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    // Struct data
    0x01, 0x23, 0x00, 0x00, 0x89, 0xab, 0xcd, 0xef
};
static const Word* SimpleStructTest = reinterpret_cast<const Word*>(SimpleStructTestBytes);

// ------------------------------------------------------------------------------------------------
alignas(8) static const uint8_t SimpleStringTestBytes[] =
{
    // List pointer, offset = 0, elementSize = 2 (1 byte), size of list = 8
    0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00,
    // List elements, ends with null terminator
    0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};
static const Word* SimpleStringTest = reinterpret_cast<const Word*>(SimpleStringTestBytes);

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, Constants)
{
    static_assert(BytesPerWord == 8);
    static_assert(BitsPerByte == 8);
    static_assert(BitsPerWord == 64);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, DataType)
{
    static_assert(DataType<bool>);
    static_assert(DataType<bool>);
    static_assert(DataType<char>);
    static_assert(DataType<char>);
    static_assert(DataType<int8_t>);
    static_assert(DataType<int8_t>);
    static_assert(DataType<int16_t>);
    static_assert(DataType<int16_t>);
    static_assert(DataType<int32_t>);
    static_assert(DataType<int32_t>);
    static_assert(DataType<int64_t>);
    static_assert(DataType<int64_t>);
    static_assert(DataType<uint8_t>);
    static_assert(DataType<uint8_t>);
    static_assert(DataType<uint16_t>);
    static_assert(DataType<uint16_t>);
    static_assert(DataType<uint32_t>);
    static_assert(DataType<uint32_t>);
    static_assert(DataType<uint64_t>);
    static_assert(DataType<uint64_t>);

    enum TestEnum { A, B, C };
    static_assert(DataType<TestEnum>);

    static_assert(DataType<Void>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, LayoutTraits)
{
    static_assert(he::IsSame<LayoutTraits<bool>::Reader, bool>);
    static_assert(he::IsSame<LayoutTraits<bool>::Builder, bool>);
    static_assert(he::IsSame<LayoutTraits<char>::Reader, char>);
    static_assert(he::IsSame<LayoutTraits<char>::Builder, char>);
    static_assert(he::IsSame<LayoutTraits<int8_t>::Reader, int8_t>);
    static_assert(he::IsSame<LayoutTraits<int8_t>::Builder, int8_t>);
    static_assert(he::IsSame<LayoutTraits<int16_t>::Reader, int16_t>);
    static_assert(he::IsSame<LayoutTraits<int16_t>::Builder, int16_t>);
    static_assert(he::IsSame<LayoutTraits<int32_t>::Reader, int32_t>);
    static_assert(he::IsSame<LayoutTraits<int32_t>::Builder, int32_t>);
    static_assert(he::IsSame<LayoutTraits<int64_t>::Reader, int64_t>);
    static_assert(he::IsSame<LayoutTraits<int64_t>::Builder, int64_t>);
    static_assert(he::IsSame<LayoutTraits<uint8_t>::Reader, uint8_t>);
    static_assert(he::IsSame<LayoutTraits<uint8_t>::Builder, uint8_t>);
    static_assert(he::IsSame<LayoutTraits<uint16_t>::Reader, uint16_t>);
    static_assert(he::IsSame<LayoutTraits<uint16_t>::Builder, uint16_t>);
    static_assert(he::IsSame<LayoutTraits<uint32_t>::Reader, uint32_t>);
    static_assert(he::IsSame<LayoutTraits<uint32_t>::Builder, uint32_t>);
    static_assert(he::IsSame<LayoutTraits<uint64_t>::Reader, uint64_t>);
    static_assert(he::IsSame<LayoutTraits<uint64_t>::Builder, uint64_t>);

    enum TestEnum { A, B, C };
    static_assert(he::IsSame<LayoutTraits<TestEnum>::Reader, TestEnum>);
    static_assert(he::IsSame<LayoutTraits<TestEnum>::Builder, TestEnum>);

    static_assert(he::IsSame<LayoutTraits<Void>::Reader, Void>);
    static_assert(he::IsSame<LayoutTraits<Void>::Builder, Void>);

    struct TestStruct { class Reader; class Builder; };
    static_assert(he::IsSame<LayoutTraits<TestStruct>::Reader, TestStruct::Reader>);
    static_assert(he::IsSame<LayoutTraits<TestStruct>::Builder, TestStruct::Builder>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, ElementSizeForByteSize)
{
    static_assert(ElementSizeForByteSize<1>::Value == ElementSize::Byte);
    static_assert(ElementSizeForByteSize<2>::Value == ElementSize::TwoBytes);
    static_assert(ElementSizeForByteSize<4>::Value == ElementSize::FourBytes);
    static_assert(ElementSizeForByteSize<8>::Value == ElementSize::EightBytes);
}

// ------------------------------------------------------------------------------------------------
struct DeclKindStructTest { static constexpr DeclKind Kind = DeclKind::Struct; };
HE_TEST(schema, layout, ElementSizeOfType)
{
    static_assert(ElementSizeOfType<bool>::Value == ElementSize::Bit);
    static_assert(ElementSizeOfType<char>::Value == ElementSize::Byte);
    static_assert(ElementSizeOfType<int8_t>::Value == ElementSize::Byte);
    static_assert(ElementSizeOfType<int16_t>::Value == ElementSize::TwoBytes);
    static_assert(ElementSizeOfType<int32_t>::Value == ElementSize::FourBytes);
    static_assert(ElementSizeOfType<int64_t>::Value == ElementSize::EightBytes);
    static_assert(ElementSizeOfType<uint8_t>::Value == ElementSize::Byte);
    static_assert(ElementSizeOfType<uint16_t>::Value == ElementSize::TwoBytes);
    static_assert(ElementSizeOfType<uint32_t>::Value == ElementSize::FourBytes);
    static_assert(ElementSizeOfType<uint64_t>::Value == ElementSize::EightBytes);

    enum TestEnum { A, B, C };
    static_assert(ElementSizeOfType<TestEnum>::Value == ElementSize::TwoBytes);

    static_assert(ElementSizeOfType<Void>::Value == ElementSize::Void);

    static_assert(ElementSizeOfType<String>::Value == ElementSize::Pointer);
    static_assert(ElementSizeOfType<List<char>>::Value == ElementSize::Pointer);
    static_assert(ElementSizeOfType<List<uint8_t>>::Value == ElementSize::Pointer);

    static_assert(ElementSizeOfType<DeclKindStructTest>::Value == ElementSize::Composite);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, PointerReader)
{
    // Default constructed
    {
        const PointerReader ptr;
        HE_EXPECT(!ptr.IsValid());
        HE_EXPECT(ptr.IsNull());
    }

    // Simple struct pointer
    {
        const PointerReader ptr(SimpleStructTest);
        HE_EXPECT(ptr.IsValid());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), (SimpleStructTest + 1));
        HE_EXPECT_EQ(ptr.StructDataWordSize(), 2);
        HE_EXPECT_EQ(ptr.StructPointerCount(), 0);
        HE_EXPECT_EQ(ptr.StructWordSize(), 2);

        const StructReader st = ptr.TryGetStruct();
        HE_EXPECT(st.IsValid());
        HE_EXPECT_EQ(st.DataWordSize(), ptr.StructDataWordSize());
        HE_EXPECT_EQ(st.PointerCount(), ptr.StructPointerCount());
    }

    // String (List<char>) pointer
    {
        const PointerReader ptr(SimpleStringTest);
        HE_EXPECT(ptr.IsValid());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), (SimpleStringTest + 1));
        HE_EXPECT_EQ(ptr.ListElementSize(), ElementSize::Byte);
        HE_EXPECT_EQ(ptr.ListSize(), 8);
        HE_EXPECT_EQ(ptr.ListCompositeTagSize(), 0);

        const ListReader list = ptr.TryGetList(ElementSize::Byte);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ(list.Size(), ptr.ListSize());
        HE_EXPECT_EQ(list.GetElementSize(), ptr.ListElementSize());
        HE_EXPECT_EQ(list.StepSize(), 8);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, ListReader)
{
    // Default constructed
    {
        const ListReader list;
        HE_EXPECT(!list.IsValid());
    }

    // List<char>
    {
        const PointerReader ptr(SimpleStringTest);
        const ListReader list = ptr.TryGetList(ElementSize::Byte);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ_PTR(list.Data(), (SimpleStringTest + 1));
        HE_EXPECT_EQ(list.Size(), 8);
        HE_EXPECT_EQ(list.StepSize(), 8);
        HE_EXPECT_EQ(list.GetElementSize(), ElementSize::Byte);
        HE_EXPECT(!list.IsEmpty());
        HE_EXPECT_EQ(list.GetDataElement<char>(0), 't');
        HE_EXPECT_EQ(list.GetDataElement<char>(1), 'e');
        HE_EXPECT_EQ(list.GetDataElement<char>(2), 's');
        HE_EXPECT_EQ(list.GetDataElement<char>(3), 't');
        HE_EXPECT_EQ(list.GetDataElement<char>(4), 'i');
        HE_EXPECT_EQ(list.GetDataElement<char>(5), 'n');
        HE_EXPECT_EQ(list.GetDataElement<char>(6), 'g');
        HE_EXPECT_EQ(list.GetDataElement<char>(7), 0);
    }

    // String
    {
        const PointerReader ptr(SimpleStringTest);
        const String::Reader str = ptr.TryGetString();
        HE_EXPECT(str.IsValid());
        HE_EXPECT_EQ_PTR(str.Data(), reinterpret_cast<const char*>(SimpleStringTest + 1));
        HE_EXPECT_EQ(str.Size(), 7);
        HE_EXPECT(!str.IsEmpty());
        HE_EXPECT_EQ(str.AsView(), "testing");
        HE_EXPECT_EQ(str[0], 't');
        HE_EXPECT_EQ(str[1], 'e');
        HE_EXPECT_EQ(str[2], 's');
        HE_EXPECT_EQ(str[3], 't');
        HE_EXPECT_EQ(str[4], 'i');
        HE_EXPECT_EQ(str[5], 'n');
        HE_EXPECT_EQ(str[6], 'g');
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, StructReader)
{
    // Default constructed
    {
        const StructReader st;
        HE_EXPECT(!st.IsValid());
    }

    // Simple struct
    {
        const PointerReader ptr(SimpleStructTest);
        const StructReader st = ptr.TryGetStruct();
        HE_EXPECT(st.IsValid());
        HE_EXPECT_EQ_PTR(st.Data(), (SimpleStructTest + 1));
        HE_EXPECT_EQ(st.DataWordSize(), 2);
        HE_EXPECT_EQ(st.PointerCount(), 0);
        HE_EXPECT_EQ(st.DataFieldCount(), 4);
        HE_EXPECT_EQ(st.HasDataField(0), true);
        HE_EXPECT_EQ(st.HasDataField(1), false);
        HE_EXPECT_EQ(st.HasDataField(2), true);
        HE_EXPECT_EQ(st.HasDataField(3), true);

        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(0, 0), 0xefcdab8900002301);
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1, 0xabcdef), 0xabcdef);

        HE_EXPECT_EQ(st.TryGetDataField<uint32_t>(0, 0), 0x00002301);
        HE_EXPECT_EQ(st.TryGetDataField<uint32_t>(1, 1), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint32_t>(2, 2), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint32_t>(2, 2, 0xabcdef), 0xabcdef);

        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 0x2301);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(1, 1), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(2, 2), 0xab89);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(3, 3), 0xefcd);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(4, 4), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(4, 4, 0xa1b2), 0xa1b2);

        HE_EXPECT(st.GetDataField<bool>(0));
        HE_EXPECT(!st.GetDataField<bool>(1));
        HE_EXPECT(!st.GetDataField<bool>(2));
        HE_EXPECT(!st.GetDataField<bool>(3));
        HE_EXPECT(!st.GetDataField<bool>(4));
        HE_EXPECT(!st.GetDataField<bool>(5));
        HE_EXPECT(!st.GetDataField<bool>(6));
        HE_EXPECT(!st.GetDataField<bool>(7));

        HE_EXPECT(st.GetDataField<bool>(8));
        HE_EXPECT(st.GetDataField<bool>(9));
        HE_EXPECT(!st.GetDataField<bool>(10));
        HE_EXPECT(!st.GetDataField<bool>(11));
        HE_EXPECT(!st.GetDataField<bool>(12));
        HE_EXPECT(st.GetDataField<bool>(13));
        HE_EXPECT(!st.GetDataField<bool>(14));
        HE_EXPECT(!st.GetDataField<bool>(15));

        HE_EXPECT(st.GetDataField<bool>(0, false));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, Builder)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, PointerBuilder)
{
    // Default construct
    {
        PointerBuilder ptr;
        HE_EXPECT(!ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.Location(), static_cast<Word*>(nullptr));
    }

    // Construct with offset
    {
        Builder b;
        PointerBuilder ptr(&b, 0);
        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), ptr.Location() + 1);
    }

    // Set Offset and Kind (struct)
    {
        Builder b;
        PointerBuilder ptr(&b, 0);
        ptr.SetOffsetAndKind(-80, PointerKind::Struct);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), -80);
        HE_EXPECT_EQ_PTR(ptr.Target(), ptr.Location() - 79);
    }

    // Set Offset and Kind (list)
    {
        Builder b;
        PointerBuilder ptr(&b, 0);
        ptr.SetOffsetAndKind(-256, PointerKind::List);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr.Offset(), -256);
        HE_EXPECT_EQ_PTR(ptr.Target(), ptr.Location() - 255);
    }

    // Set Offset and Kind (zero struct)
    {
        Builder b;
        PointerBuilder ptr(&b, 0);
        ptr.SetOffsetAndKind(-1, PointerKind::Struct);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), -1);
        HE_EXPECT_EQ_PTR(ptr.Target(), ptr.Location() + 0);
    }

    // Set List
    {
        Builder b;
        PointerBuilder ptr(&b, 0);

        ListBuilder list = b.AddList(ElementSize::Byte, 16);
        ptr.Set(list);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), list.Location());
        HE_EXPECT_EQ(ptr.ListElementSize(), list.GetElementSize());
        HE_EXPECT_EQ(ptr.ListSize(), list.Size());

        HE_EXPECT_VERIFY({
            HE_EXPECT(!ptr.TryGetStruct().IsValid());
        });
        HE_EXPECT_VERIFY({
            HE_EXPECT(!ptr.TryGetList(ElementSize::TwoBytes).IsValid());
        });
        HE_EXPECT_VERIFY({
            HE_EXPECT(!ptr.TryGetList(ElementSize::Composite).IsValid());
        });

        ListBuilder list2 = ptr.TryGetList(ElementSize::Byte);
        HE_EXPECT(list2.IsValid());
        HE_EXPECT_EQ(list2.GetElementSize(), list.GetElementSize());
        HE_EXPECT_EQ(list2.Size(), list.Size());
        HE_EXPECT_EQ_PTR(list2.Data(), list.Data());

        String::Builder str = ptr.TryGetString();
        HE_EXPECT(str.IsValid());
        HE_EXPECT_EQ(str.GetElementSize(), list.GetElementSize());
        HE_EXPECT_EQ(str.Size(), list.Size() - 1);
        HE_EXPECT_EQ_PTR(str.Data(), reinterpret_cast<char*>(list.Data()));

        List<uint8_t>::Builder list4 = ptr.TryGetList<uint8_t>();
        HE_EXPECT(list4.IsValid());
        HE_EXPECT_EQ(list4.GetElementSize(), list.GetElementSize());
        HE_EXPECT_EQ(list4.Size(), list.Size());
        HE_EXPECT_EQ_PTR(list4.Data(), reinterpret_cast<uint8_t*>(list.Data()));
    }

    // Set Struct
    {
        Builder b;
        PointerBuilder ptr(&b, 0);

        StructBuilder st = b.AddStruct(4, 16, 1);
        ptr.Set(st);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), st.Location());
        HE_EXPECT_EQ(ptr.StructDataWordSize(), st.DataWordSize());
        HE_EXPECT_EQ(ptr.StructPointerCount(), st.PointerCount());

        HE_EXPECT_VERIFY({
            HE_EXPECT(!ptr.TryGetList(ElementSize::Byte).IsValid());
        });
        HE_EXPECT_VERIFY({
            HE_EXPECT(!ptr.TryGetList(ElementSize::Composite).IsValid());
        });

        StructReader st2 = ptr.TryGetStruct();
        HE_EXPECT(st2.IsValid());
        HE_EXPECT_EQ(st2.DataWordSize(), st.DataWordSize());
        HE_EXPECT_EQ(st2.PointerCount(), st.PointerCount());
    }

    // Copy Null
    {
        Builder b;
        PointerBuilder ptr(&b, 0);
        ptr.SetOffsetAndKind(10, PointerKind::List);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr.Offset(), 10);
        HE_EXPECT_EQ_PTR(ptr.Target(), ptr.Location() + 11);

        b.AddStruct(0, 0, 1);
        PointerBuilder ptr2(&b, 1);
        HE_EXPECT(ptr2.IsValid());
        HE_EXPECT(ptr2.IsNull());

        ptr.Copy(ptr2);
        HE_EXPECT(ptr.IsValid());
        HE_EXPECT(ptr.IsNull());
    }

    // Copy List
    {
        Builder b;
        PointerBuilder ptr(&b, 0);

        ListBuilder list = b.AddList(ElementSize::Byte, 16);
        ptr.Set(list);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), list.Location());

        HE_EXPECT(b.Size(), 3);

        PointerBuilder ptr2 = b.AddPointer();
        ptr2.Copy(ptr);

        HE_EXPECT(ptr2.IsValid());
        HE_EXPECT_EQ_PTR(ptr2.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr2.Location(), b.Data() + 3);
        HE_EXPECT(!ptr2.IsNull());
        HE_EXPECT(!ptr2.IsZeroStruct());
        HE_EXPECT_EQ(ptr2.Kind(), PointerKind::List);
        HE_EXPECT_EQ(ptr2.Offset(), 0);
        HE_EXPECT_NE_PTR(ptr2.Target(), list.Location());

        HE_EXPECT(b.Size(), 5);

        ListBuilder list2 = ptr2.TryGetList(ElementSize::Byte);
        HE_EXPECT_NE_PTR(list2.Location(), list.Location());
        HE_EXPECT_EQ(list2.GetElementSize(), list.GetElementSize());
        HE_EXPECT_EQ(list2.Size(), list.Size());
        HE_EXPECT_EQ_MEM(list2.Location(), list.Location(), list.Size());
    }

    // Copy Struct
    {
        Builder b;
        PointerBuilder ptr(&b, 0);

        StructBuilder st = b.AddStruct(4, 4, 0);
        ptr.Set(st);

        HE_EXPECT(ptr.IsValid());
        HE_EXPECT_EQ_PTR(ptr.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr.Location(), b.Data());
        HE_EXPECT(!ptr.IsNull());
        HE_EXPECT(!ptr.IsZeroStruct());
        HE_EXPECT_EQ(ptr.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr.Offset(), 0);
        HE_EXPECT_EQ_PTR(ptr.Target(), st.Location());

        HE_EXPECT(b.Size(), 5);

        PointerBuilder ptr2 = b.AddPointer();
        ptr2.Copy(ptr);

        HE_EXPECT(ptr2.IsValid());
        HE_EXPECT_EQ_PTR(ptr2.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(ptr2.Location(), b.Data() + 5);
        HE_EXPECT(!ptr2.IsNull());
        HE_EXPECT(!ptr2.IsZeroStruct());
        HE_EXPECT_EQ(ptr2.Kind(), PointerKind::Struct);
        HE_EXPECT_EQ(ptr2.Offset(), 0);
        HE_EXPECT_NE_PTR(ptr2.Target(), st.Location());

        HE_EXPECT(b.Size(), 9);

        StructBuilder st2 = ptr2.TryGetStruct();
        HE_EXPECT_NE_PTR(st2.Location(), st.Location());
        HE_EXPECT_EQ(st2.DataFieldCount(), st.DataFieldCount());
        HE_EXPECT_EQ(st2.DataWordSize(), st.DataWordSize());
        HE_EXPECT_EQ(st2.PointerCount(), st.PointerCount());
        HE_EXPECT_EQ_MEM(st2.Location(), st2.Location(), (st.DataWordSize() + st.PointerCount()) * BytesPerWord);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, ListBuilder)
{
    // Default construct
    {
        ListBuilder list;
        HE_EXPECT(!list.IsValid());
        HE_EXPECT_EQ_PTR(list.Location(), static_cast<Word*>(nullptr));
    }

    // Primitive elements
    {
        Builder b;
        ListBuilder list = b.AddList<uint16_t>(8);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ_PTR(list.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(list.Location(), b.Data() + 1);
        HE_EXPECT_EQ_PTR(list.Data(), b.Data() + 1);
        HE_EXPECT_EQ(list.Size(), 8);
        HE_EXPECT_EQ(list.StepSize(), 16);
        HE_EXPECT_EQ(list.StructDataFieldCount(), 0);
        HE_EXPECT_EQ(list.GetElementSize(), ElementSize::TwoBytes);
        HE_EXPECT(!list.IsEmpty());
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            HE_EXPECT_EQ(list.GetDataElement<uint16_t>(i), 0);
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            list.SetDataElement<uint16_t>(i, i);
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            HE_EXPECT_EQ(list.GetDataElement<uint16_t>(i), i);
        }
    }

    // Pointer elements
    {
        constexpr he::StringView TestString{ "testing" };

        Builder b;
        ListBuilder list = b.AddList<String>(5);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ_PTR(list.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(list.Location(), b.Data() + 1);
        HE_EXPECT_EQ_PTR(list.Data(), b.Data() + 1);
        HE_EXPECT_EQ(list.Size(), 5);
        HE_EXPECT_EQ(list.StepSize(), 64);
        HE_EXPECT_EQ(list.StructDataFieldCount(), 0);
        HE_EXPECT_EQ(list.GetElementSize(), ElementSize::Pointer);
        HE_EXPECT(!list.IsEmpty());
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            PointerBuilder ptr = list.GetPointerElement(i);
            HE_EXPECT(ptr.IsValid());
            HE_EXPECT(ptr.IsNull());
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            list.SetPointerElement(i, b.AddString(TestString));
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            PointerBuilder ptr = list.GetPointerElement(i);
            HE_EXPECT(ptr.IsValid());
            HE_EXPECT(!ptr.IsNull());
            String::Builder str = ptr.TryGetString();
            HE_EXPECT(str.IsValid());
            HE_EXPECT_EQ(str.AsView(), TestString);
        }
    }

    // Composite elements
    {
        constexpr he::StringView TestString{ "testing" };

        Builder b;
        ListBuilder list = b.AddStructList(4, 8, 2, 4);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ_PTR(list.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(list.Location(), b.Data() + 2);
        HE_EXPECT_EQ_PTR(list.Data(), b.Data() + 2);
        HE_EXPECT_EQ_PTR(list.Tag().Location(), b.Data() + 1);
        HE_EXPECT_EQ(list.Size(), 4);
        HE_EXPECT_EQ(list.StepSize(), 64 * 6);
        HE_EXPECT_EQ(list.StructDataFieldCount(), 8);
        HE_EXPECT_EQ(list.StructDataWordSize(), 2);
        HE_EXPECT_EQ(list.StructPointerCount(), 4);
        HE_EXPECT_EQ(list.GetElementSize(), ElementSize::Composite);
        HE_EXPECT(!list.IsEmpty());
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            StructBuilder st = list.GetCompositeElement(i);
            HE_EXPECT(st.IsValid());
            HE_EXPECT_EQ(st.DataFieldCount(), list.StructDataFieldCount());
            HE_EXPECT_EQ(st.DataWordSize(), list.StructDataWordSize());
            HE_EXPECT_EQ(st.PointerCount(), list.StructPointerCount());

            HE_EXPECT(!st.HasDataField(0));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 0);
            HE_EXPECT(!st.HasDataField(1));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(1, 1), 0);
            HE_EXPECT(!st.HasDataField(2));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(2, 2), 0);
            HE_EXPECT(!st.HasDataField(3));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(3, 3), 0);
            HE_EXPECT(!st.HasDataField(4));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(4, 4), 0);
            HE_EXPECT(!st.HasDataField(5));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(5, 5), 0);
            HE_EXPECT(!st.HasDataField(6));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(6, 6), 0);
            HE_EXPECT(!st.HasDataField(7));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(7, 7), 0);
            HE_EXPECT(!st.HasDataField(8));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(8, 8), 0);
            HE_EXPECT(!st.HasDataField(32));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(32, 32, 156), 156);

            HE_EXPECT(!st.HasPointerField(0));
            HE_EXPECT(st.GetPointerField(0).IsNull());
            HE_EXPECT(!st.HasPointerField(1));
            HE_EXPECT(st.GetPointerField(1).IsNull());
            HE_EXPECT(!st.HasPointerField(2));
            HE_EXPECT(st.GetPointerField(2).IsNull());
            HE_EXPECT(!st.HasPointerField(3));
            HE_EXPECT(st.GetPointerField(3).IsNull());
            HE_EXPECT(!st.HasPointerField(32));
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            StructBuilder st = list.GetCompositeElement(i);
            HE_EXPECT(st.IsValid());
            st.SetAndMarkDataField<uint16_t>(0, 0, 12);
            st.SetAndMarkDataField<uint16_t>(1, 1, 34);
            st.SetAndMarkDataField<uint16_t>(2, 2, 56);
            st.SetAndMarkDataField<uint16_t>(3, 3, 78);
            st.GetPointerField(0).Set(b.AddString(TestString));
        }
        for (uint16_t i = 0; i < list.Size(); ++i)
        {
            StructBuilder st = list.GetCompositeElement(i);
            HE_EXPECT(st.IsValid());
            HE_EXPECT_EQ(st.DataFieldCount(), list.StructDataFieldCount());
            HE_EXPECT_EQ(st.DataWordSize(), list.StructDataWordSize());
            HE_EXPECT_EQ(st.PointerCount(), list.StructPointerCount());

            HE_EXPECT(st.HasDataField(0));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 12);
            HE_EXPECT(st.HasDataField(1));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(1, 1), 34);
            HE_EXPECT(st.HasDataField(2));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(2, 2), 56);
            HE_EXPECT(st.HasDataField(3));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(3, 3), 78);
            HE_EXPECT(!st.HasDataField(4));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(4, 4), 0);
            HE_EXPECT(!st.HasDataField(5));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(5, 5), 0);
            HE_EXPECT(!st.HasDataField(6));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(6, 6), 0);
            HE_EXPECT(!st.HasDataField(7));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(7, 7), 0);
            HE_EXPECT(!st.HasDataField(8));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(8, 8), 0);
            HE_EXPECT(!st.HasDataField(32));
            HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(32, 32, 156), 156);

            HE_EXPECT(st.HasPointerField(0));
            HE_EXPECT(!st.GetPointerField(0).IsNull());
            HE_EXPECT_EQ(st.GetPointerField(0).TryGetString().AsView(), TestString);
            HE_EXPECT(!st.HasPointerField(1));
            HE_EXPECT(st.GetPointerField(1).IsNull());
            HE_EXPECT(!st.HasPointerField(2));
            HE_EXPECT(st.GetPointerField(2).IsNull());
            HE_EXPECT(!st.HasPointerField(3));
            HE_EXPECT(st.GetPointerField(3).IsNull());
            HE_EXPECT(!st.HasPointerField(32));
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, StructBuilder)
{
    // Default construct
    {
        StructBuilder st;
        HE_EXPECT(!st.IsValid());
        HE_EXPECT_EQ_PTR(st.Location(), static_cast<Word*>(nullptr));
    }

    // Basic construct
    {
        Builder b;
        StructBuilder st = b.AddStruct(19, 4, 13);
        HE_EXPECT(st.IsValid());
        HE_EXPECT_EQ_PTR(st.GetBuilder(), &b);
        HE_EXPECT_EQ_PTR(st.Location(), b.Data() + 1);
        HE_EXPECT_EQ(st.DataFieldCount(), 19);
        HE_EXPECT_EQ(st.DataWordSize(), 4);
        HE_EXPECT_EQ(st.PointerCount(), 13);
        for (uint16_t i = 0; i < 32; ++i)
        {
            HE_EXPECT(!st.HasDataField(i));
        }
        for (uint16_t i = 0; i < 16; ++i)
        {
            HE_EXPECT(!st.HasPointerField(i));
        }
    }

    // Simple test create
    {
        Builder b;
        StructBuilder st = b.AddStruct(4, 2, 0);
        HE_EXPECT(st.IsValid());
        st.SetAndMarkDataField<uint16_t>(0, 0, 0x2301);
        st.SetAndMarkDataField<uint16_t>(2, 2, 0xab89);
        st.SetAndMarkDataField<uint16_t>(3, 3, 0xefcd);

        b.SetRoot(st);
        HE_EXPECT_EQ(b.Size() * BytesPerWord, sizeof(SimpleStructTestBytes));
        HE_EXPECT_EQ_MEM(b.Data(), SimpleStructTestBytes, sizeof(SimpleStructTestBytes));
    }

    {
        Builder b;
        StructBuilder st = b.AddStruct(4, 4, 4);
        HE_EXPECT(st.IsValid());

        HE_EXPECT(!st.HasDataField(0));
        HE_EXPECT(!st.HasDataField(1));
        HE_EXPECT(!st.HasDataField(2));
        HE_EXPECT(!st.HasDataField(3));
        HE_EXPECT(!st.HasPointerField(0));
        HE_EXPECT(!st.HasPointerField(1));
        HE_EXPECT(!st.HasPointerField(2));
        HE_EXPECT(!st.HasPointerField(3));

        // Data fields

        HE_EXPECT(!st.HasDataField(0));
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0, 51), 51);
        st.SetAndMarkDataField<uint16_t>(0, 0, 12345);
        HE_EXPECT(st.HasDataField(0));
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 12345);
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0, 51), 12345);

        HE_EXPECT(!st.HasDataField(1));
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1), 0);
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1, 51), 51);
        st.SetAndMarkDataField<uint64_t>(1, 1, 987654321);
        HE_EXPECT(st.HasDataField(1));
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1), 987654321);
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1, 51), 987654321);

        HE_EXPECT(!st.HasDataField(2));
        HE_EXPECT_EQ(st.TryGetDataField<bool>(2, 16), false);
        HE_EXPECT_EQ(st.TryGetDataField<bool>(2, 16, true), true);
        st.SetAndMarkDataField<bool>(2, 16, true);
        HE_EXPECT(st.HasDataField(2));
        HE_EXPECT_EQ(st.TryGetDataField<bool>(2, 16), true);
        HE_EXPECT_EQ(st.TryGetDataField<bool>(2, 16, false), true);

        HE_EXPECT(!st.HasDataField(3));
        he::Span<uint32_t> v = st.GetAndMarkDataArrayField<uint32_t>(3, 4, 2);
        HE_EXPECT(st.HasDataField(3));
        HE_EXPECT_EQ(v.Size(), 2);
        v[0] = 1;
        v[1] = 2;

        // Pointers

        HE_EXPECT(!st.HasPointerField(0));
        PointerBuilder p0 = st.GetPointerField(0);
        HE_EXPECT(p0.IsValid());
        HE_EXPECT(p0.IsNull());
        p0.Set(b.AddString("test0"));
        HE_EXPECT(!p0.IsNull());
        HE_EXPECT(st.HasPointerField(0));

        HE_EXPECT(!st.HasPointerField(1));
        PointerBuilder p1 = st.GetPointerField(1);
        HE_EXPECT(p1.IsValid());
        HE_EXPECT(p1.IsNull());
        p1.Set(b.AddStruct(5, 4, 3));
        HE_EXPECT(!p1.IsNull());
        HE_EXPECT(st.HasPointerField(1));

        HE_EXPECT(!st.HasPointerField(2));
        ListBuilder p2 = st.GetPointerArrayField(2, 2);
        HE_EXPECT(p2.IsValid());
        HE_EXPECT_EQ(p2.Size(), 2);
        HE_EXPECT(p2.GetPointerElement(0).IsValid());
        HE_EXPECT(p2.GetPointerElement(0).IsNull());
        p2.GetPointerElement(0).Set(b.AddString("test1"));
        HE_EXPECT(!p2.GetPointerElement(0).IsNull());

        HE_EXPECT(p2.GetPointerElement(1).IsValid());
        HE_EXPECT(p2.GetPointerElement(1).IsNull());
        p2.GetPointerElement(1).Set(b.AddString("test2"));
        HE_EXPECT(!p2.GetPointerElement(1).IsNull());

        // Check that writes didn't mess up any data
        HE_EXPECT_EQ(st.TryGetDataField<uint16_t>(0, 0), 12345);
        HE_EXPECT_EQ(st.TryGetDataField<uint64_t>(1, 1), 987654321);
        HE_EXPECT_EQ(st.TryGetDataField<bool>(2, 64), true);
        HE_EXPECT_EQ(st.GetAndMarkDataArrayField<uint32_t>(3, 5, 3).Size(), 3);
        HE_EXPECT_EQ(st.GetPointerField(0).TryGetString().AsView(), "test0");
        StructBuilder st_p1_st = st.GetPointerField(1).TryGetStruct();
        HE_EXPECT_EQ(st_p1_st.DataFieldCount(), 5);
        HE_EXPECT_EQ(st_p1_st.DataWordSize(), 4);
        HE_EXPECT_EQ(st_p1_st.PointerCount(), 3);
        HE_EXPECT_EQ(st.GetPointerField(2).TryGetString().AsView(), "test1");
        HE_EXPECT_EQ(st.GetPointerField(3).TryGetString().AsView(), "test2");

        // Check that clear works
        st.ClearAllFields();
        HE_EXPECT(!st.HasDataField(0));
        HE_EXPECT(!st.HasDataField(1));
        HE_EXPECT(!st.HasDataField(2));
        HE_EXPECT(!st.HasDataField(3));
        HE_EXPECT(!st.HasPointerField(0));
        HE_EXPECT(!st.HasPointerField(1));
        HE_EXPECT(!st.HasPointerField(2));
        HE_EXPECT(!st.HasPointerField(3));
    }

    // Copy
    {
        // TODO
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, String_Reader)
{
    // Default construct
    {
        String::Reader str;
        HE_EXPECT(!str.IsValid());
    }

    // Simple string read
    {
        String::Reader str(ListReader(SimpleStringTest + 1, 8, 8, ElementSize::Byte));
        HE_EXPECT(str.IsValid());
        HE_EXPECT_EQ(str.Size(), 7);
        HE_EXPECT_EQ(str.AsView(), "testing");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, String_Builder)
{
    // Default construct
    {
        String::Builder str;
        HE_EXPECT(!str.IsValid());
    }

    // Simple string create
    {
        Builder b;
        String::Builder str = b.AddString("testing");
        HE_EXPECT(str.IsValid());
        HE_EXPECT_EQ_PTR(str.Location(), b.Data() + 1);
        HE_EXPECT_EQ(str.Size(), 7);
        HE_EXPECT_EQ(str.AsView(), "testing");

        b.SetRoot(str);
        HE_EXPECT_EQ(b.Size() * BytesPerWord, sizeof(SimpleStringTestBytes));
        HE_EXPECT_EQ_MEM(b.Data(), SimpleStringTestBytes, sizeof(SimpleStringTestBytes));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, ListIterator)
{
    constexpr char ExpectedStr[] = "testing";

    List<char>::Reader list(ListReader(SimpleStringTest + 1, 8, 8, ElementSize::Byte));
    uint32_t index = 0;
    for (const char c : list)
    {
        HE_EXPECT_LT(index, sizeof(ExpectedStr));
        HE_EXPECT_EQ(c, ExpectedStr[index]);
        ++index;
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, StringIterator)
{
    constexpr char ExpectedStr[] = "testing";

    String::Reader str(ListReader(SimpleStringTest + 1, 8, 8, ElementSize::Byte));
    uint32_t index = 0;
    for (const char c : str)
    {
        HE_EXPECT_LT(index, sizeof(ExpectedStr) - 1);
        HE_EXPECT_EQ(c, ExpectedStr[index]);
        ++index;
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, List_Reader)
{
    // Default construct
    {
        List<char>::Reader list;
        HE_EXPECT(!list.IsValid());
    }

    // Simple string read
    {
        List<char>::Reader list(ListReader(SimpleStringTest + 1, 8, 8, ElementSize::Byte));
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ(list.Size(), 8);
        HE_EXPECT_EQ(list[0], 't');
        HE_EXPECT_EQ(list[1], 'e');
        HE_EXPECT_EQ(list[2], 's');
        HE_EXPECT_EQ(list[3], 't');
        HE_EXPECT_EQ(list[4], 'i');
        HE_EXPECT_EQ(list[5], 'n');
        HE_EXPECT_EQ(list[6], 'g');
        HE_EXPECT_EQ(list[7], '\0');
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, List_Builder)
{
    // Default construct
    {
        List<char>::Builder list;
        HE_EXPECT(!list.IsValid());
    }

    // Simple string create
    {
        Builder b;
        List<char>::Builder list = b.AddList<char>(8);
        HE_EXPECT(list.IsValid());
        HE_EXPECT_EQ_PTR(list.Location(), b.Data() + 1);
        HE_EXPECT_EQ(list.Size(), 8);
        list.Set(0, 't');
        list.Set(1, 'e');
        list.Set(2, 's');
        list.Set(3, 't');
        list.Set(4, 'i');
        list.Set(5, 'n');
        list.Set(6, 'g');
        list.Set(7, '\0');

        b.SetRoot(list);
        HE_EXPECT_EQ(b.Size() * BytesPerWord, sizeof(SimpleStringTestBytes));
        HE_EXPECT_EQ_MEM(b.Data(), SimpleStringTestBytes, sizeof(SimpleStringTestBytes));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, layout, Builder_Roundtrip)
{
    // TODO
}
