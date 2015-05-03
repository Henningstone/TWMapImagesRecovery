/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* Modified by unsigned char* */
#include "datafile.h"
#include <zlib.h>
#include <cstring>
#include <cstdlib>

bool CDataFileReader::open(const char *pFilename)
{
	std::ifstream *pFile = new std::ifstream(pFilename, std::ios::in|std::ios::binary);
	if(!pFile->is_open())
		return false;


	// TODO: change this header
	CDatafileHeader Header;
	pFile->read((char*)&Header, sizeof(CDatafileHeader));
	if(Header.m_aID[0] != 'A' || Header.m_aID[1] != 'T' || Header.m_aID[2] != 'A' || Header.m_aID[3] != 'D')
	{
		if(Header.m_aID[0] != 'D' || Header.m_aID[1] != 'A' || Header.m_aID[2] != 'T' || Header.m_aID[3] != 'A')
			return 0;
	}

	#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
	#endif

	if(Header.m_Version != 3 && Header.m_Version != 4)
		return 0;

	// read in the rest except the data
	unsigned Size = 0;
	Size += Header.m_NumItemTypes*sizeof(CDatafileItemType);
	Size += (Header.m_NumItems+Header.m_NumRawData)*sizeof(int);
	if(Header.m_Version == 4)
		Size += Header.m_NumRawData*sizeof(int); // v4 has uncompressed data sizes aswell
	Size += Header.m_ItemSize;

	unsigned AllocSize = Size;
	AllocSize += sizeof(CDatafile); // add space for info structure
	AllocSize += Header.m_NumRawData*sizeof(void*); // add space for data pointers

	CDatafile *pTmpDataFile = (CDatafile*)malloc(AllocSize);
	pTmpDataFile->m_Header = Header;
	pTmpDataFile->m_DataStartOffset = sizeof(CDatafileHeader) + Size;
	pTmpDataFile->m_ppDataPtrs = (char**)(pTmpDataFile+1);
	pTmpDataFile->m_pData = (char *)(pTmpDataFile+1)+Header.m_NumRawData*sizeof(char *);
	pTmpDataFile->m_pFile = pFile;

	// clear the data pointers
	memset(pTmpDataFile->m_ppDataPtrs, 0x0, Header.m_NumRawData*sizeof(void*));

	// read types, offsets, sizes and item data
	pFile->read(pTmpDataFile->m_pData, Size);
	if(!(*pFile))
	{
		pTmpDataFile->m_pFile->close();
		delete pTmpDataFile->m_pFile;
		free(pTmpDataFile);
		pTmpDataFile = 0;
		return false;
	}

	close();
	m_pDataFile = pTmpDataFile;

	#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(m_pDataFile->m_pData, sizeof(int), min(static_cast<unsigned>(Header.m_Swaplen), Size) / sizeof(int));
	#endif

	m_pDataFile->m_Info.m_pItemTypes = (CDatafileItemType *)m_pDataFile->m_pData;
	m_pDataFile->m_Info.m_pItemOffsets = (int *)&m_pDataFile->m_Info.m_pItemTypes[m_pDataFile->m_Header.m_NumItemTypes];
	m_pDataFile->m_Info.m_pDataOffsets = (int *)&m_pDataFile->m_Info.m_pItemOffsets[m_pDataFile->m_Header.m_NumItems];
	m_pDataFile->m_Info.m_pDataSizes = (int *)&m_pDataFile->m_Info.m_pDataOffsets[m_pDataFile->m_Header.m_NumRawData];

	if(Header.m_Version == 4)
		m_pDataFile->m_Info.m_pItemStart = (char *)&m_pDataFile->m_Info.m_pDataSizes[m_pDataFile->m_Header.m_NumRawData];
	else
		m_pDataFile->m_Info.m_pItemStart = (char *)&m_pDataFile->m_Info.m_pDataOffsets[m_pDataFile->m_Header.m_NumRawData];
	m_pDataFile->m_Info.m_pDataStart = m_pDataFile->m_Info.m_pItemStart + m_pDataFile->m_Header.m_ItemSize;

	return true;
}


int CDataFileReader::numData()
{
	if(!m_pDataFile) { return 0; }
	return m_pDataFile->m_Header.m_NumRawData;
}

// always returns the size in the file
int CDataFileReader::getDataSize(int Index)
{
	if(!m_pDataFile) { return 0; }

	if(Index == m_pDataFile->m_Header.m_NumRawData-1)
		return m_pDataFile->m_Header.m_DataSize-m_pDataFile->m_Info.m_pDataOffsets[Index];

	return m_pDataFile->m_Info.m_pDataOffsets[Index+1]-m_pDataFile->m_Info.m_pDataOffsets[Index];
}

long CDataFileReader::getDataSizeUncompressed(int Index)
{
	return m_pDataFile->m_Info.m_pDataSizes[Index];
}

void *CDataFileReader::getDataImpl(int Index, int Swap)
{
	if(!m_pDataFile) { return 0; }

	// load it if needed
	if(!m_pDataFile->m_ppDataPtrs[Index])
	{
		// fetch the data size
		int DataSize = getDataSize(Index);
		#if defined(CONF_ARCH_ENDIAN_BIG)
			int SwapSize = DataSize;
		#endif

		if(m_pDataFile->m_Header.m_Version == 4)
		{
			// v4 has compressed data
			void *pTemp = (char *)malloc(DataSize);
			unsigned long UncompressedSize = m_pDataFile->m_Info.m_pDataSizes[Index];
			unsigned long s;

			m_pDataFile->m_ppDataPtrs[Index] = (char *)malloc(UncompressedSize);

			// read the compressed data
			m_pDataFile->m_pFile->seekg(m_pDataFile->m_DataStartOffset+m_pDataFile->m_Info.m_pDataOffsets[Index], std::ios_base::beg);
			m_pDataFile->m_pFile->read((char*)pTemp, DataSize);

			// decompress the data, TODO: check for errors
			s = UncompressedSize;
			uncompress((Bytef*)m_pDataFile->m_ppDataPtrs[Index], &s, (Bytef*)pTemp, DataSize); // ignore_convention
			#if defined(CONF_ARCH_ENDIAN_BIG)
				SwapSize = s;
			#endif

			// clean up the temporary buffers
			free(pTemp);
		}
		else
		{
			// load the data
			m_pDataFile->m_ppDataPtrs[Index] = (char *)malloc(DataSize);
			m_pDataFile->m_pFile->seekg(m_pDataFile->m_DataStartOffset+m_pDataFile->m_Info.m_pDataOffsets[Index], std::ios_base::beg);
			m_pDataFile->m_pFile->read(m_pDataFile->m_ppDataPtrs[Index], DataSize);
		}

		#if defined(CONF_ARCH_ENDIAN_BIG)
			if(Swap && SwapSize)
				swap_endian(m_pDataFile->m_ppDataPtrs[Index], sizeof(int), SwapSize/sizeof(int));
		#endif
	}

	return m_pDataFile->m_ppDataPtrs[Index];
}

void *CDataFileReader::getData(int Index)
{
	return getDataImpl(Index, 0);
}

void *CDataFileReader::getDataSwapped(int Index)
{
	return getDataImpl(Index, 1);
}

void CDataFileReader::unloadData(int Index)
{
	if(Index < 0)
		return;

	//
	free(m_pDataFile->m_ppDataPtrs[Index]);
	m_pDataFile->m_ppDataPtrs[Index] = 0x0;
}

int CDataFileReader::getItemSize(int Index)
{
	if(!m_pDataFile) { return 0; }
	if(Index == m_pDataFile->m_Header.m_NumItems-1)
		return m_pDataFile->m_Header.m_ItemSize-m_pDataFile->m_Info.m_pItemOffsets[Index];
	return m_pDataFile->m_Info.m_pItemOffsets[Index+1]-m_pDataFile->m_Info.m_pItemOffsets[Index];
}

void *CDataFileReader::getItem(int Index, int *pType, int *pID)
{
	if(!m_pDataFile) { if(pType) *pType = 0; if(pID) *pID = 0; return 0; }

	CDatafileItem *i = (CDatafileItem *)(m_pDataFile->m_Info.m_pItemStart+m_pDataFile->m_Info.m_pItemOffsets[Index]);
	if(pType)
		*pType = (i->m_TypeAndID>>16)&0xffff; // remove sign extention
	if(pID)
		*pID = i->m_TypeAndID&0xffff;
	return (void *)(i+1);
}

void CDataFileReader::getType(int Type, int *pStart, int *pNum)
{
	*pStart = 0;
	*pNum = 0;

	if(!m_pDataFile)
		return;

	for(int i = 0; i < m_pDataFile->m_Header.m_NumItemTypes; i++)
	{
		if(m_pDataFile->m_Info.m_pItemTypes[i].m_Type == Type)
		{
			*pStart = m_pDataFile->m_Info.m_pItemTypes[i].m_Start;
			*pNum = m_pDataFile->m_Info.m_pItemTypes[i].m_Num;
			return;
		}
	}
}

void *CDataFileReader::findItem(int Type, int ID)
{
	if(!m_pDataFile) return 0;

	int Start, Num;
	getType(Type, &Start, &Num);
	for(int i = 0; i < Num; i++)
	{
		int ItemID;
		void *pItem = getItem(Start+i,0, &ItemID);
		if(ID == ItemID)
			return pItem;
	}
	return 0;
}

int CDataFileReader::numItems()
{
	if(!m_pDataFile) return 0;
	return m_pDataFile->m_Header.m_NumItems;
}

bool CDataFileReader::close()
{
	if(!m_pDataFile)
		return true;

	// free the data that is loaded
	int i;
	for(i = 0; i < m_pDataFile->m_Header.m_NumRawData; i++)
		free(m_pDataFile->m_ppDataPtrs[i]);

	m_pDataFile->m_pFile->close();
	delete m_pDataFile->m_pFile;
	free(m_pDataFile);
	m_pDataFile = 0;
	return true;
}

unsigned CDataFileReader::crc()
{
	if(!m_pDataFile) return 0xFFFFFFFF;
	return m_pDataFile->m_Crc;
}
