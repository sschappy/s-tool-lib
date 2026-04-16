class ByteStream
{
public:
    ByteStream();
    explicit ByteStream(uint64_t bufSize);

    uint64_t size() const
    {
        return m_size;
    }

    uint64_t capacity() const
    {
        return m_capacity;
    }

    uint64_t tell() const
    {
        return m_curPos;
    }

    void seek(uint64_t pos)
    {
        if (pos <= m_size)
        {
            m_curPos = pos;
        }
    }

    void* data() const
    {
        return m_data.get();
    }

    void reset();
    void resetPosition();
    void resetSize(uint64_t newSize);

    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    ByteStream& operator<< (const T& t);

    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    ByteStream& operator>> (T& t);

    ByteStream& operator<< (const std::string& s);
    ByteStream& operator>> (std::string& s);

    ByteStream& write(void* pData, uint64_t size);
    ByteStream& read(void* pData, uint64_t size);

private:
    void adjustCapacity(uint64_t newCapacity);

private:
    uint64_t m_size{ 0 };
    uint64_t m_capacity{ 10 };
    uint64_t m_curPos{ 0 };
    std::unique_ptr<uint8_t[]> m_data;
};

ByteStream::ByteStream()
    : m_data(std::make_unique<uint8_t[]>(m_capacity))
{}

ByteStream::ByteStream(uint64_t bufSize)
    : m_capacity(bufSize)
    , m_data(std::make_unique<uint8_t[]>(m_capacity))
{}

void ByteStream::reset()
{
    m_size = 0;
    m_curPos = 0;
}

void ByteStream::resetPosition()
{
    m_curPos = 0;
}

void ByteStream::resetSize(uint64_t newSize)
{
    if (newSize > m_capacity)
    {
        adjustCapacity(newSize);
    }
    m_size = newSize;
    m_curPos = newSize;
}

void ByteStream::adjustCapacity(uint64_t newCapacity)
{
    if (newCapacity == m_capacity)
    {
        return;
    }

    auto newData = std::make_unique<uint8_t[]>(newCapacity);
    if (nullptr != m_data)
    {
        std::copy_n(m_data.get(), std::min(m_size, newCapacity), newData.get());
    }
    m_data = std::move(newData);
    m_capacity = newCapacity;
}

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int>>
ByteStream& ByteStream::operator<< (const T& t)
{
    if (m_curPos + sizeof(T) > m_capacity)
    {
        adjustCapacity(std::max(m_curPos + sizeof(T), m_capacity * 2));
    }

    memcpy(&m_data[m_curPos], &t, sizeof(T));
    m_curPos += sizeof(T);
    m_size = std::max(m_curPos, m_size);
    return *this;
}

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int>>
ByteStream& ByteStream::operator>> (T& t)
{
    if (m_curPos + sizeof(T) <= m_size)
    {
        memcpy(&t, &m_data[m_curPos], sizeof(T));
        m_curPos += sizeof(T);
    }
    return *this;
}

ByteStream& ByteStream::operator<< (const std::string& s)
{
    if (m_curPos + s.size() + sizeof(uint32_t) > m_capacity)
    {
        adjustCapacity(std::max(m_curPos + s.size() + sizeof(uint32_t), m_capacity * 2));
    }
    const auto size = static_cast<uint32_t>(s.size());
    (*this) << size;
    if (0 != size)
    {
        memcpy(&m_data[m_curPos], s.data(), size);
        m_curPos += size;
    }
    m_size = std::max(m_curPos, m_size);
    return *this;
}

ByteStream& ByteStream::operator>> (std::string& s)
{
    if (m_curPos + sizeof(uint32_t) <= m_size)
    {
        uint32_t size = 0;
        (*this) >> size;
        s.resize(size);
        if (0 != size && m_curPos + size <= m_size)
        {
            memcpy(const_cast<char*>(s.data()), &m_data[m_curPos], size);
            m_curPos += size;
        }
    }
    return *this;
}

ByteStream& ByteStream::write(void* pData, uint64_t size)
{
    if (m_curPos + size > m_capacity)
    {
        adjustCapacity(std::max(m_curPos + size, m_capacity * 2));
    }
    memcpy(&m_data[m_curPos], pData, size);
    m_curPos += size;
    m_size = std::max(m_curPos, m_size);
    return *this;
}

ByteStream& ByteStream::read(void* pData, uint64_t size)
{
    if (m_curPos + size <= m_size)
    {
        memcpy(pData, &m_data[m_curPos], size);
        m_curPos += size;
    }
    return *this;
}
