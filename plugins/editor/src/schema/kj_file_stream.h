// Copyright Chad Engler

#include "he/core/assert.h"
#include "he/core/file.h"
#include "he/core/result.h"
#include "he/core/types.h"

#include "capnp/serialize.h"

namespace he::editor
{
    class KjFileOutputStream : public kj::OutputStream
    {
    public:
        KjFileOutputStream(File& f) : m_file(f) {}

        void write(const void* buffer, size_t size) override
        {
            const Result r = m_file.Write(buffer, static_cast<uint32_t>(size));
            HE_ASSERT_RESULT(r);
        }

        File& m_file;
    };

    class KjFileInputStream : public kj::InputStream
    {
    public:
        KjFileInputStream(File& f) : m_file(f) {}

        size_t tryRead(void* buffer, size_t minBytes, size_t maxBytes) override
        {
            //const Result r = m_file.Read(buffer, maxBytes)
            uint8_t* pos = static_cast<uint8_t*>(buffer);
            uint8_t* min = pos + minBytes;
            uint8_t* max = pos + maxBytes;

            while (pos < min)
            {
                uint32_t bytesRead = 0;
                const uint32_t bytesToRead = static_cast<uint32_t>(max - pos);
                const Result r = m_file.Read(pos, bytesToRead, &bytesRead);
                HE_ASSERT_RESULT(r);

                if (bytesToRead == bytesRead)
                    break;

                pos += bytesRead;
            }

            return pos - static_cast<uint8_t*>(buffer);
        }

        File& m_file;
    };
}
