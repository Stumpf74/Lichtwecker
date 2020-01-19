//: ////////////////////////////////////////////////////////////////////////////
//+
//+       (C) ITF-EDV Fr�schl GmbH, Hauserbachstr. 8, 93194 Walderbach
//+
//+ ////////////////////////////////////////////////////////////////////////////
//+
//+ Project:... Lib
//+ Module:...  Average
//+  Bemerkung:
//+
//. ////////////////////////////////////////////////////////////////////////////
#ifndef INC_AVERAGE_H
#define INC_AVERAGE_H

////////////////////////////////////////////////////////////////////////////////
//                      definements and global variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                             forward declaration
////////////////////////////////////////////////////////////////////////////////

/// Creates a template for average objects
/// @remarks
/// @author  JuHe 04.2012
template <class T>
class CAverage
{
public:
   ////////////////////////////////////////////////////////////////////////////////
   //                           constructors & destructor
   ////////////////////////////////////////////////////////////////////////////////

   CAverage(uint32_t ui_buffer_size, T InitValueBuffer)
   {
      m_ptrausData = new T[ui_buffer_size];
      m_ptrWriteData = m_ptrausData;
      m_uiBufferSize = ui_buffer_size;
      InitBuffer(InitValueBuffer);
   }

   ~CAverage()
   {
      delete[] m_ptrausData;
      m_ptrausData = NULL;
   }

   ////////////////////////////////////////////////////////////////////////////////
   //                                    functions
   ////////////////////////////////////////////////////////////////////////////////
public:
   void Add(T nextdata)
   {
      *m_ptrWriteData++ = nextdata;
      if (m_ptrWriteData >= (m_ptrausData + m_uiBufferSize))
         m_ptrWriteData = m_ptrausData;
   }

   T Get(void)
   {
      double tAverage = 0;

      for (uint32_t i = 0; i < m_uiBufferSize; i++)
      {
         tAverage += m_ptrausData[i];
      }

      tAverage = tAverage / m_uiBufferSize;

      return (T)tAverage;
   }

   void InitBuffer(T initvalue)
   {
      if (m_ptrausData != 0 && m_uiBufferSize > 0)
      {
         for (uint32_t i = 0; i < m_uiBufferSize; i++)
         {
            m_ptrausData[i] = initvalue;
         }
      }
   }

private:
   T *m_ptrausData;
   T *m_ptrWriteData;
   uint32_t m_uiBufferSize;
};

#endif
