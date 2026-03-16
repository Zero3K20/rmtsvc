/*******************************************************************
   *	Buffer.h
   *    DESCRIPTION:Circular buffer and linear buffer
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-08-19
   *	net4cpp 2.1
   *******************************************************************/
   
#ifndef __YY_CBUFFER_H__
#define __YY_CBUFFER_H__

namespace net4cpp21
{
	class cLoopBuffer
	{
	public:
		explicit cLoopBuffer(size_t size);
		~cLoopBuffer();
		//Append l bytes to the buffer
		bool Write(const char *p,size_t l);
		//Read l bytes into the dest buffer
		bool Read(char *dest,size_t l);
		/** skip l bytes from buffer */
		bool Remove(size_t l);

		/** total buffer length */
		size_t GetLength() { return m_q; }
		/** pointer to circular buffer beginning */
		char *GetStart() { return buf + m_b; }
		/** return number of bytes from circular buffer beginning to buffer physical end */
		size_t GetL() { return (m_b + m_q > m_max) ? m_max - m_b : m_q; }
		/** return free space in buffer, number of bytes until buffer overrun */
		size_t Space() { return m_max - m_q; }

		/** return total number of bytes written to this buffer, ever */
		unsigned long ByteCounter() { return m_count; }

private:
	char *buf; //Buffer
	size_t m_max;//Buffer capacity
	size_t m_q; //Data size in buffer
	size_t m_b; //Read start position in buffer
	size_t m_t; //Write start position in buffer
	unsigned long m_count; //Total bytes written to buffer
	};

	class cBuffer
	{
	public:
		explicit cBuffer(size_t size=0);
		~cBuffer();
		
		cBuffer(cBuffer &buf);
		cBuffer & operator = (cBuffer &buf);
		char *str() { return m_buf;}
		size_t &len() { return m_len;}
		char & operator [] (size_t pos);
		size_t size() { return m_max; }
		size_t Space() { return m_max - m_len; } //Remaining space
		char * Resize(size_t size);
		
	private:
		char *m_buf; //Buffer
		size_t m_max;//Buffer capacity
		size_t m_len; //Data size in buffer
	};
}//?namespace net4cpp21

#endif

